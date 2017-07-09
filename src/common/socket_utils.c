#include "socket_utils.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

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

#ifdef __unix
	int optval = 1;
	if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (void *) &optval, sizeof(optval))) {
		perr_sock("Error socket");
		exit(1);
	}
#endif

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

Socket connect_socket(unsigned long addr, u_short port) {
    struct sockaddr_in server;

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = addr;
    server.sin_port = port;

    Socket sock = socket(AF_INET, SOCK_STREAM, 0);
    if(!is_socket_valid(sock)) {
        perr_sock("Error: creating socket");
        socket_cleanup();
        exit(1);
    }
    if(connect(sock, (struct sockaddr *) &server, sizeof(server))) {
        perr_sock("Error bind");
        socket_cleanup();
        exit(1);
    }
    return sock;
}
