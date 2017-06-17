#include "cryptorserver.h"
#include "socket.h"
#include "utilsCompat.h"
#include "logging.h"
#include "error.h"
#include "protocol.h"

#include <stdlib.h>
#include <string.h>

void cryptor_handle_connection(Socket client) {
    char cmd[5];
    memset(cmd, '\0', sizeof(cmd));
    recv(client, cmd, sizeof(cmd) - 1, MSG_WAITALL);

    logf("Received command %s\n", cmd);

    if(strcmp(cmd, ENCR) == 0) {
        char cmdline[MAX_CMDLINE_LEN - CMD_LEN + 1]; //we have already read the command, so subtract its size
        memset(cmdline, 0, sizeof(cmdline));
        recv(client, cmdline, sizeof(cmdline) - 1, MSG_WAITALL);
        logf("Rest of command line %s\n", cmdline);
    }

    send(client, "200", 3, 0);

    socket_close(client);
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
