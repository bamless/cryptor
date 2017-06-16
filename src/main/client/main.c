#include "socket.h"
#include "logging.h"
#include "error.h"

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

static void usage(char *exec_name);
static void parse_args(int argc, char **argv, char *cmd);
static void connect_sock(Socket *sock, const char *host);
static void close_and_exit(Socket *sock);

static int send_command(Socket *sock, const char *cmd);

int main(int argc, char **argv) {
	char cmd;
	parse_args(argc, argv, &cmd);

	socket_startup();

	Socket sock;
	connect_sock(&sock , argv[optind]);

	int resp_code = 0;
	switch(cmd) {
		case 'l':
			resp_code = send_command(&sock, "LSTF");
			break;
	}

	logf("Server responded with code %d\n", resp_code);

	socket_close(sock);
	socket_cleanup();
}

static int send_command(Socket *sock, const char *cmd) {
	//send command
	if(send(*sock, cmd, 4, 0) == -1) {
		perr_sock("Error: send_command");
		close_and_exit(sock);
	}

	//read response code
	char resp[4];
	int received;
    memset(resp, '\0', sizeof(resp));
    if((received = recv(*sock, resp, sizeof(resp) - 1, MSG_WAITALL)) == -1) {
		perr_sock("Error: send_command");
		close_and_exit(sock);
	}
	if(received == 0) {
		elog("Socket closed by foreign host");
		close_and_exit(sock);
	}

	return (int) strtol(resp, NULL, 0);
}

static void connect_sock(Socket *sock, const char *host) {
	struct sockaddr_in server;

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(host);
	server.sin_port = htons(8888);

	*sock = socket(AF_INET, SOCK_STREAM, 0);
	if(!is_socket_valid(*sock)) {
		perr_sock("Error: creating socket");
		close_and_exit(sock);
	}
	if(connect(*sock, (struct sockaddr *) &server, sizeof(server))) {
		perr_sock("Error: bind");
		close_and_exit(sock);
	}
}

static void parse_args(int argc, char **argv, char *cmd) {
	switch ((*cmd = getopt(argc, argv, "lRed"))) {
		case 'l':
			if(argc != 3) usage(argv[0]);
			break;
		case 'R':
			if(argc != 3) usage(argv[0]);
			break;
		case 'e':
			if(argc != 5) usage(argv[0]);
			break;
		case 'd':
			if(argc != 5) usage(argv[0]);
			break;
		case '?':
			if(isprint(optopt))
				elogf("Unknown option `-%c`.\n", optopt);
			else
				elogf("Unknown option character `\\x%x`.\n", optopt);
				usage(argv[0]);
			break;
	}
}

static void close_and_exit(Socket *sock) {
	socket_close(*sock);
	socket_cleanup();
	exit(1);
}

static void usage(char *exec_name) {
	elogf("Usage: %s [-l | -R | -e seed path | -d seed path] ip\n", exec_name);
	exit(1);
}
