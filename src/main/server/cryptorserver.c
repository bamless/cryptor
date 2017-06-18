#include "cryptorserver.h"
#include "socket.h"
#include "utilsCompat.h"
#include "logging.h"
#include "error.h"
#include "stringbuf.h"
#include "files.h"
#include "protocol.h"

#include "fileUtils.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static void handle_list_commands(Socket client, int is_recursive);

void cryptor_handle_connection(Socket client) {
    char cmd[5];
    memset(cmd, '\0', sizeof(cmd));
    recv(client, cmd, sizeof(cmd) - 1, MSG_WAITALL);

    if(strcmp(cmd, LSTF) == 0) {
        handle_list_commands(client, 0);
    } else if(strcmp(cmd, LSTR) == 0) {
        handle_list_commands(client, 1);
    } else if(strcmp(cmd, ENCR) == 0) {
        send(client, RETOK, 3, 0); //TODO: implement
    } else if(strcmp(cmd, DECR) == 0) {
        send(client, RETOK, 3, 0); //TODO: implement
    }

    socket_close(client);
}

/* Appends the line "fsize file_path\r\n\r\n" in the string buffer for every file in the PWD.
 * If is_recursive is non-zero then then it explores recursevely all the subfolders.*/
static void list(const char *path, StringBuffer *sb, int is_recursive) {
    int err;
    Dir *dir = open_dir(path, &err);
    if(!dir || err) return;

    DirEntry entry;
    while(has_next(dir)) {
        next_dir(dir, &entry);
        if(strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0) {
            if(entry.type == NFILE) {
                char full_file_path[MAX_PATH_LENGTH];
                snprintf(full_file_path, sizeof(full_file_path), "%s/%s", path, entry.name);
                fsize_t fsize;
                if(get_file_size(full_file_path, &fsize)) continue;
                char fsize_str[20]; //20 the max length of 64 bit int in base 10
                snprintf(fsize_str, sizeof(fsize_str), "%"PRIu64" ", (uintmax_t) fsize); //we can safely cast to uintmax_t because get_file_size guarantees a result >= 0
                sbuf_appendstr(sb, fsize_str);
                sbuf_appendstr(sb, path);
                sbuf_appendstr(sb, "/");
                sbuf_appendstr(sb, entry.name);
                sbuf_appendstr(sb, "\r\n");
            }
            if(entry.type == DIRECTORY && is_recursive) {
                char subdir_path[MAX_PATH_LENGTH];
                snprintf(subdir_path, MAX_PATH_LENGTH, "%s/%s", path, entry.name);
                list(subdir_path, sb, is_recursive);
            }
        }
    }
    close_dir(dir);
}

static void handle_list_commands(Socket client, int is_recursive) {
    StringBuffer *sb = sbuf_create();
    char pwd[MAX_PATH_LENGTH + 1];
    if(get_cwd(pwd, sizeof(pwd))) {
        send(client, RETERR, 3, 0);
        return;
    }

    list(pwd, sb, is_recursive);
    if(strcmp("", sbuf_get_backing_buf(sb)) == 0) { //the pwd is empty
        send(client, RETERR, 3, 0);
    } else {
        send(client, RETMORE, 3, 0);
        sbuf_appendstr(sb, "\r\n");
        send(client, sbuf_get_backing_buf(sb), sbuf_get_len(sb), 0);
    }
    sbuf_destroy(sb);
}

Socket init_server_socket(u_short port) {
	struct sockaddr_in server;

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = port;

	Socket server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(!is_socket_valid(server_sock)) {
		perr_sock("Error: creating socket");
		socket_cleanup();
		exit(1);
	}
	if(bind(server_sock, (struct sockaddr *) &server, sizeof(server))) {
		perr_sock("Error: bind server");
		socket_cleanup();
		exit(1);
	}
	if(listen(server_sock, SOMAXCONN)) {
		perr_sock("Error: bind server");
		socket_cleanup();
		exit(1);
	}
    return server_sock;
}
