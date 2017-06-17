#include "socket.h"
#include "logging.h"
#include "error.h"
#include "protocol.h"
#include "cryptorclient.h"

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

typedef struct ParsedArgs {
	char cmd;
	unsigned long host_addr;
	u_short host_port;
	unsigned int seed;
	const char *path;
} ParsedArgs;

static void usage(char *exec_name);
static void parse_args(int argc, char **argv, ParsedArgs *args);
static void strtoipandport(char *hostandport, unsigned long *ip, u_short *port);

int main(int argc, char **argv) {
	ParsedArgs args;
	parse_args(argc, argv, &args);

	socket_startup();

	Socket sock = init_connection(args.host_addr, args.host_port);

	int resp_code = 0;
	switch(args.cmd) {
		case 'l':
			resp_code = cryptor_send_command(sock, LSTF, 0, NULL);
			break;
	}

	logf("Server responded with code %d\n", resp_code);

	socket_close(sock);
	socket_cleanup();
}

static void parse_args(int argc, char **argv, ParsedArgs *args) {
	int c;
	if(argc < 3) usage(argv[0]);
	switch ((c = getopt(argc, argv, "lRed"))) {
		case 'l':
		case 'R':
			if(argc != 3) usage(argv[0]);
			args->cmd = c;
			strtoipandport(argv[optind], &args->host_addr, &args->host_port);
			if(args->host_addr < 0) usage(argv[0]);
			if(args->host_port == 0) args->host_port = htons(DEFAULT_PORT);
			break;
		case 'e':
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

/*
 * Parses a string of the form ipaddr:port and returns the ip addr in the "ip" arg
 * and the port in the "port" arg.
 * The "ip" arg will be < 0 if the function fails to parse the ip address, while
 * the "port" arg will be 0 if the function fails to parse the port.
 */
static void strtoipandport(char *hostandport, unsigned long *ip, u_short *port) {
	char *hoststr = strtok(hostandport, ":");
	*ip = inet_addr(hoststr);

	char *portstr = strtok(NULL, ":");
	if(portstr == NULL) {
		*port = 0;
		return;
	}

	char *err;
	long p = strtol(portstr, &err, 10);
	if(*err != '\0' || p < PORT_MIN || p > PORT_MAX)
		p = 0;
	*port = (u_short) htons((uint16_t) p);
}

static void usage(char *exec_name) {
	elogf("Usage: %s [-l | -R | -e seed path | -d seed path] ip:port\n", exec_name);
	exit(1);
}
