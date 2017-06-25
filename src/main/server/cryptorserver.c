#include "cryptorserver.h"
#include "socket.h"
#include "error.h"
#include "stringbuf.h"
#include "files.h"
#include "logging.h"
#include "protocol.h"
#include "encrypt.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#ifdef _WIN32
static int rand_r(unsigned int *seed);
#endif

static void handle_list_commands(Socket client, int is_recursive);
static void handle_encrytion_commands(Socket client, int is_decrypt);

void cryptor_handle_connection(Socket client) {
    char cmd[5];
    memset(cmd, '\0', sizeof(cmd));
    recv(client, cmd, sizeof(cmd) - 1, MSG_WAITALL);

    dlogf("Received command %s\n", cmd);

    if(strcmp(cmd, LSTF) == 0) {
        handle_list_commands(client, 0);
    } else if(strcmp(cmd, LSTR) == 0) {
        handle_list_commands(client, 1);
    } else if(strcmp(cmd, ENCR) == 0){
        handle_encrytion_commands(client, 0);
    } else if(strcmp(cmd, DECR) == 0) {
        handle_encrytion_commands(client, 1);
    }

    socket_close(client);
}


static void send_list(Socket s, StringBuffer *path, StringBuffer *cmdline, int is_recursive);

static void handle_list_commands(Socket client, int is_recursive) {
    char *pwd = get_cwd();

    StringBuffer *path = sbuf_create();
    StringBuffer *cmdline = sbuf_create();
    sbuf_appendstr(path, pwd);
    free(pwd);

    send(client, RETMORE, 3, MSG_NOSIGNAL);
    send_list(client, path, cmdline, is_recursive); //sends the directory list
    send(client, "\r\n", 2, MSG_NOSIGNAL); //signal end of output (\r\n\r\n)

    sbuf_destroy(path);
    sbuf_destroy(cmdline);
}

/* Sends the line "fsize file_path\r\n" for every file in the PWD. If is_recursive
 * is non-zero then then it explores recursevely all the subfolders.*/
static void send_list(Socket s, StringBuffer *path, StringBuffer *cmdline, int is_recursive) {
    int err;
    Dir *dir = open_dir(sbuf_get_backing_buf(path), &err);
    if(!dir || err) {
        perr("Error send_list");
        return;
    }

    DirEntry entry;
    while(has_next(dir)) {
        next_dir(dir, &entry);
        if(strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0) {
            int orig_len = sbuf_get_len(path); //save the prev length of the path
            //append the entry name to obtain the full path
            sbuf_appendstr(path, "/");
            sbuf_appendstr(path, entry.name);

            if(entry.type == NFILE) {
                fsize_t fsize;
                if(get_file_size(sbuf_get_backing_buf(path), &fsize)) {
                    sbuf_truncate(path, orig_len);
                    continue;
                }
                sbuf_clear(cmdline);
                char fsizestr[21]; //20 max size of 64 bit integer + 1 for NUL
                //we can safely cast to uintmax_t because get_file_size guarantees a result >= 0
                snprintf(fsizestr, sizeof(fsizestr), "%"PRIu64" ", (uintmax_t) fsize);

                sbuf_appendstr(cmdline, fsizestr);                   //the size of the file
                sbuf_appendstr(cmdline, sbuf_get_backing_buf(path)); //its path
                sbuf_appendstr(cmdline, "\r\n");                     //carriage return and newline

                if(send(s, sbuf_get_backing_buf(cmdline), sbuf_get_len(cmdline), MSG_NOSIGNAL) < 0)
                    return;
            }
            if(entry.type == DIRECTORY && is_recursive) {
                send_list(s, path, cmdline, is_recursive);
            }
            sbuf_truncate(path, orig_len); //remove the entry name from the path
        }
    }
    close_dir(dir);
}

static int parse_encryption_cmdline(Socket client, unsigned int *seed, char **path);
static void generate_key(unsigned int seed, int *key, int key_len);
static void change_name(const char *name, int is_decrypt);

static void handle_encrytion_commands(Socket client, int is_decrypt) {
    char *path = NULL;
    unsigned int seed;
    if(parse_encryption_cmdline(client, &seed, &path)) {
        send(client, RETERR, 3, MSG_NOSIGNAL);
        if(path) free(path);
        return;
    }

    int err;
    File file = open_file(path, READ | WRITE, &err);
    if(err) {
        char *errn = (err == ERR_NOFILE) ? RETERR : RETERRTRANS;
        send(client, errn, 3, MSG_NOSIGNAL);
        free(path);
        return;
    }

    fsize_t s;
    if(fget_file_size(file, &s)) {
        send(client, RETERRTRANS, 3, MSG_NOSIGNAL);
        close_file(file);
        free(path);
        return;
    }
    s = ceil(s/4.) * 4; //closest multiple of 4 greater than size

    if(lock_file(file, 0, s)) {
        send(client, RETERRTRANS, 3, MSG_NOSIGNAL);
        close_file(file);
        free(path);
        return;
    }

    int key[8];
    generate_key(seed, key, 8);
    if(encrypt(file, key, 8)) {
        send(client, RETERRTRANS, 3, MSG_NOSIGNAL);
        unlock_file(file, 0, s);
        close_file(file);
        free(path);
        return;
    }

    unlock_file(file, 0, s);
    close_file(file);

    change_name(path, is_decrypt);
    free(path);

    send(client, RETOK, 3, MSG_NOSIGNAL);
}

static int parse_encryption_cmdline(Socket client, unsigned int *seed, char **path) {
    StringBuffer *cmdline = sbuf_create();
    char buff[512];
    ssize_t bytes_recv;
    while((bytes_recv = recv(client, buff, sizeof(buff), 0)) > 0) {
        sbuf_append(cmdline, buff, bytes_recv);
        if(sbuf_endswith(cmdline, "\r\n")) break; //\r\n signals end of encr/decr command lines
    }
    sbuf_truncate(cmdline, sbuf_get_len(cmdline) - 2); //remove the \r\n
    size_t cmdline_len = sbuf_get_len(cmdline);

    char *saveptr, *err;
    char *seed_str = strtok_r(sbuf_get_backing_buf(cmdline), " ", &saveptr);

    unsigned long seedl = strtoul(seed_str, &err, 10);
    if(seedl > UINT_MAX || *err) {
        sbuf_destroy(cmdline);
        return - 1;
    }
    *seed = (unsigned int) seedl;
    dlogf("seed: %d\n", *seed);

    char *tok;
    *path = malloc(cmdline_len - (strlen(seed_str) + 1) + 1);
    strcpy(*path, strtok_r(NULL, " ", &saveptr));
    while((tok = strtok_r(NULL, " ", &saveptr))) {
        strcat(*path, tok);
    }

    dlogf("file: %s\n", *path);
    sbuf_destroy(cmdline);
    return 0;
}

static void generate_key(unsigned int seed, int *key, int key_len) {
    for(int i = 0; i < key_len; i++) {
        key[i] = rand_r(&seed);
    }
}

static void change_name(const char *name, int is_decrypt) {
    char *new;
    if(is_decrypt) {
        int len = strlen(name) - 3;
        new = malloc(len);
        strncpy(new, name, len - 1);
        new[len - 1] = '\0';
    } else {
        new = malloc(strlen(name) + 5);
        strcpy(new, name);
        strcat(new, "_enc");
    }
    rename_file(name, new);
    free(new);
}

#ifdef _WIN32
static int rand_r(unsigned int *seed) {
        long k;
        long s = (long)(*seed);
        if (s == 0)
            s = 0x12345987;
        k = s / 127773;
        s = 16807 * (s - k * 127773) - 2836 * k;
        if (s < 0)
            s += 2147483647;
        (*seed) = (unsigned int)s;
        return (int)(s & RAND_MAX);
}
#endif

Socket init_server_socket(u_short port) {
	struct sockaddr_in server;

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = port;

	Socket server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(!is_socket_valid(server_sock)) {
		perr_sock("Error creating socket");
		socket_cleanup();
		exit(1);
	}
	if(bind(server_sock, (struct sockaddr *) &server, sizeof(server))) {
		perr_sock("Error bind");
		socket_cleanup();
		exit(1);
	}
	if(listen(server_sock, SOMAXCONN)) {
		perr_sock("Error bind");
		socket_cleanup();
		exit(1);
	}
    return server_sock;
}
