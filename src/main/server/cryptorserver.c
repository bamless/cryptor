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

static void handle_list_commands(Socket client, int is_recursive);
static void handle_encrytion_commands(Socket client);

void cryptor_handle_connection(Socket client) {
    char cmd[5];
    memset(cmd, '\0', sizeof(cmd));
    recv(client, cmd, sizeof(cmd) - 1, MSG_WAITALL);

    dlogf("Received command %s\n", cmd);

    if(strcmp(cmd, LSTF) == 0) {
        handle_list_commands(client, 0);
    } else if(strcmp(cmd, LSTR) == 0) {
        handle_list_commands(client, 1);
    } else if(strcmp(cmd, ENCR) == 0 || strcmp(cmd, DECR) == 0) {
        handle_encrytion_commands(client);
    }

    socket_close(client);
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

                sbuf_appendstr(cmdline, fsizestr); //the size of the file
                sbuf_appendstr(cmdline, sbuf_get_backing_buf(path)); //its path
                sbuf_appendstr(cmdline, "\r\n"); // carriage return and newline

                if(send(s, sbuf_get_backing_buf(cmdline), sbuf_get_len(cmdline), MSG_NOSIGNAL) < 0) {
                        return;
                }
            }
            if(entry.type == DIRECTORY && is_recursive) {
                send_list(s, path, cmdline, is_recursive);
            }
            sbuf_truncate(path, orig_len); //remove the entry name from the path
        }
    }
    close_dir(dir);
}

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

static int parse_encryption_cmdline(Socket client, unsigned int *seed, char **path);
static void generate_key(unsigned int seed, int *key, int key_len);

static void handle_encrytion_commands(Socket client) {
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
    free(path);

	fsize_t s;
	if(fget_file_size(file, &s)) {
        send(client, RETERRTRANS, 3, MSG_NOSIGNAL);
        return;
	}
    s = ceil(s/4.) * 4; //closest multiple of 4 greater than size

    if(lock_file(file, 0, s)) {
        send(client, RETERRTRANS, 3, MSG_NOSIGNAL);
        return;
    }

    int key[8];
    generate_key(seed, key, 8);
    err = encrypt(file, key, 8);

    err |= unlock_file(file, 0, s);

    if(err) {
        send(client, RETERRTRANS, 3, MSG_NOSIGNAL);
        return;
    }

    send(client, RETOK, 3, MSG_NOSIGNAL);
}

static int parse_encryption_cmdline(Socket client, unsigned int *seed, char **path) {
    StringBuffer *sb = sbuf_create();
    char buff[512];
    ssize_t bytes_recv;
    while((bytes_recv = recv(client, buff, sizeof(buff), 0)) > 0) {
        sbuf_append(sb, buff, bytes_recv);
        if(sbuf_endswith(sb, "\r\n")) break; //\r\n signals end of encr/decr command lines
    }
    sbuf_truncate(sb, sbuf_get_len(sb) - 2);

    char *saveptr;
    char *seed_str = strtok_r(sbuf_get_backing_buf(sb), " ", &saveptr);

    char *err;
    unsigned long seedl = strtoul(seed_str, &err, 10);
    if(seedl > UINT_MAX || *err) {
        sbuf_destroy(sb);
        return - 1;
    }
    *seed = (unsigned int) seedl;
    dlogf("seed: %d\n", *seed);

    char *tok;
    *path = malloc(sbuf_get_len(sb) - sizeof(seed_str) + 1);
    strcpy(*path, strtok_r(NULL, " ", &saveptr));
    while((tok = strtok_r(NULL, " ", &saveptr)))
        strcat(*path, tok);

    dlogf("file: %s\n", *path);
    sbuf_destroy(sb);
    return 0;
}

static void generate_key(unsigned int seed, int *key, int key_len) {
    for(int i = 0; i < key_len; i++) {
        key[i] = rand_r(&seed);
    }
}

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
