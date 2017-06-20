#include "socket.h"
#include "logging.h"
#include "error.h"
#include "stringbuf.h"
#include "protocol.h"
#include "cryptorclient.h"

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <inttypes.h>

typedef struct ParsedArgs {
	char cmd;				 /*The command passed from arguments*/
	unsigned long host_addr; /*The server address*/
	u_short host_port;		 /*The server port*/
	unsigned int seed;		 /*The seed for the ENCR and DECR command*/
	const char *path;		 /*The path for the ENCR and DECR command*/
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
		case 'R' :
			resp_code = cryptor_send_command(sock, LSTR, 0, NULL);
			break;
		case 'e':
			resp_code = cryptor_send_command(sock, ENCR, args.seed, args.path);
			break;
		case 'd':
			resp_code = cryptor_send_command(sock, DECR, args.seed, args.path);
			break;
	}

	logsf("Server responded with code %d\n", resp_code);

	//If the response of the server is 300 (i.e. the server will send more output) read the remaining output
	if(resp_code == RETMORE_INT) {
		StringBuffer *sb = sbuf_create();
		cryptor_print_more(sock);
		sbuf_destroy(sb);
	}

	socket_close(sock);
	socket_cleanup();
}

static void parse_args(int argc, char **argv, ParsedArgs *args) {
	if(argc < 3) usage(argv[0]);
	//get the ip addr and the port
	strtoipandport(argv[argc - 1], &args->host_addr, &args->host_port);
	if(args->host_addr < 0) usage(argv[0]);
	if(args->host_port == 0) args->host_port = htons(DEFAULT_PORT);

	int c;
	switch ((c = getopt(argc, argv, "lRed"))) {
		case 'l':
		case 'R':
			if(argc != 3) usage(argv[0]);
			args->cmd = c; //the command
			break;
		case 'e':
		case 'd': {
			if(argc != 5) usage(argv[0]);
			args->cmd = c; //the command
			//convert the 'seed' argument
			char *err;
			unsigned long seed = strtol(argv[optind], &err, 10);
			if(*err != '\0' || seed > UINT_MAX) usage(argv[0]);
			args->seed = (unsigned int) seed;
			//sets the 'path' argument
			args->path = argv[optind + 1];
			break;
		}
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
	if(*ip == INADDR_NONE) *ip = -1;

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
	elogf("Usage: %s [-l | -R | -e seed path | -d seed path] ipaddr:port\n", exec_name);
	exit(1);
}
