#include "cryptorclient.h"
#include "socket.h"
#include "error.h"
#include "protocol.h"
#include "logging.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void close_and_exit(Socket sock);
static int read_response(Socket sock);

int cryptor_send_command(Socket sock, const char *cmd, unsigned int seed, const char *path) {
    if(strcmp(cmd, LSTF) == 0 || strcmp(cmd, LSTR) == 0) {
        //send command
        if(send(sock, cmd, 4, 0) == -1) {
            perr_sock("Error: send_command");
            close_and_exit(sock);
        }
    } else if(strcmp(cmd, ENCR) == 0 || strcmp(cmd, DECR) == 0) {
        if(strlen(path) > MAX_PROT_PATH) {
            elog("Path too long");
            close_and_exit(sock);
        }
        int max_cmdline_len = MAX_PROT_PATH + UINT_LEN + CMD_LEN + 3; //the 3 is for the 2 spaces and the null terminator
        char cmdstr[max_cmdline_len];
        memset(cmdstr, 0, sizeof(cmdstr));
        snprintf(cmdstr, max_cmdline_len, "%s %u %s", cmd, seed, path);
        if(send(sock, cmdstr, max_cmdline_len, 0) == -1) {
            perr_sock("Error: send_command");
            close_and_exit(sock);
        }
    } else {
        elog("Error: cryptor_send_command: unknown command");
        close_and_exit(sock);
    }

    return read_response(sock);
}

Socket init_connection(unsigned long addr, u_short port) {
    struct sockaddr_in server;

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = addr;
    server.sin_port = port;

    Socket sock = socket(AF_INET, SOCK_STREAM, 0);
    if(!is_socket_valid(sock)) {
        perr_sock("Error: creating socket");
        close_and_exit(sock);
    }
    if(connect(sock, (struct sockaddr *) &server, sizeof(server))) {
        perr_sock("Error: bind");
        close_and_exit(sock);
    }
    return sock;
}

static int read_response(Socket sock) {
    //read response code
    char resp[4];
    int received;
    memset(resp, '\0', sizeof(resp));
    if((received = recv(sock, resp, sizeof(resp) - 1, MSG_WAITALL)) == -1) {
        perr_sock("Error: send_command");
        close_and_exit(sock);
    }
    if(received == 0) {
        elog("Socket closed by foreign host");
        close_and_exit(sock);
    }

    return (int) strtol(resp, NULL, 0);
}

static void close_and_exit(Socket sock) {
	socket_close(sock);
	socket_cleanup();
	exit(1);
}
