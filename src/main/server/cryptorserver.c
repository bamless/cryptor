#include "cryptorserver.h"
#include "socket.h"
#include "logging.h"
#include "error.h"
#include "stringbuf.h"
#include "files.h"
#include "protocol.h"

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

/* Sends the line "fsize file_path\r\n" for every file in the PWD. If is_recursive
 * is non-zero then then it explores recursevely all the subfolders.*/
static void send_list(Socket s, StringBuffer *path, StringBuffer *cmdline, int is_recursive) {
    int err;
    Dir *dir = open_dir(sbuf_get_backing_buf(path), &err);
    if(!dir || err) return;

    DirEntry entry;
    while(has_next(dir)) {
        next_dir(dir, &entry);
        if(strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0) {
            int orig_len = sbuf_get_len(path);     //save the prev length of the path
            sbuf_appendf(path, "/%s", entry.name); //append the entry name to obtain the full path
            if(entry.type == NFILE) {
                fsize_t fsize;
                if(get_file_size(sbuf_get_backing_buf(path), &fsize)) {
                    sbuf_truncate(path, orig_len);
                    continue;
                }
                sbuf_clear(cmdline);
                //we can safely cast to uintmax_t because get_file_size guarantees a result >= 0
                sbuf_appendf(cmdline, "%"PRIu64" %s\r\n", (uintmax_t) fsize, sbuf_get_backing_buf(path));
                send(s, sbuf_get_backing_buf(cmdline), sbuf_get_len(cmdline), 0);
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

    send(client, RETMORE, 3, 0);
    send_list(client, path, cmdline, is_recursive); //sends the directory list
    send(client, "\r\n", 2, 0); //signal end of output (\r\n\r\n)

    sbuf_destroy(path);
    sbuf_destroy(cmdline);
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
