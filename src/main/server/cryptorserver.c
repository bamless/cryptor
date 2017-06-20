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

/* Sends the line "fsize file_path\r\n" for every file in the PWD. If is_recursive
 * is non-zero then then it explores recursevely all the subfolders.*/
static void send_list(Socket s, const char *path, int is_recursive) {
    int err;
    Dir *dir = open_dir(path, &err);
    if(!dir || err) return;

    DirEntry entry;
    while(has_next(dir)) {
        next_dir(dir, &entry);
        if(strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0) {
            char full_entry_path[MAX_PATH_LENGTH + 1];
            snprintf(full_entry_path, MAX_PATH_LENGTH, "%s/%s", path, entry.name);
            full_entry_path[MAX_PATH_LENGTH] = '\0'; //just in case it was truncated
            if(entry.type == NFILE) {
                fsize_t fsize;
                if(get_file_size(full_entry_path, &fsize)) continue;
                char line[MAX_PATH_LENGTH + 20 + 3]; //20 the max length of 64 bit int in base 10, 3 for \r\n and NUL term
                //we can safely cast to uintmax_t because get_file_size guarantees a result >= 0
                int written = snprintf(line, sizeof(line) - 1, "%"PRIu64" %s\r\n", (uintmax_t) fsize, full_entry_path);
                send(s, line, written, 0);
            }
            if(entry.type == DIRECTORY && is_recursive) {
                send_list(s, full_entry_path, is_recursive);
            }
        }
    }
    close_dir(dir);
}

static void handle_list_commands(Socket client, int is_recursive) {
    char pwd[MAX_PATH_LENGTH + 1];
    if(get_cwd(pwd, sizeof(pwd))) {
        send(client, RETERR, 3, 0);
        return;
    }

    send(client, RETMORE, 3, 0);
    send_list(client, pwd, is_recursive); //sends the directory list
    send(client, "\r\n", 2, 0); //signal end of output (\r\n\r\n)
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
