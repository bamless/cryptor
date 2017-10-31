#include "socket.h"
#include "logging.h"
#include "error.h"
#include "stringbuf.h"
#include "thread.h"
#include "socket_utils.h"
#include "protocol.h"
#include "cryptorclient.h"

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <inttypes.h>

#define SEEDS_FILE "cryptor_seeds"

/**Struct that holds the arguments passed to the program*/
typedef struct ParsedArgs {
	const char *cmd;         /*The command passed from arguments*/
	unsigned long host_addr; /*The server address*/
	u_short host_port;       /*The server port*/
	unsigned int seed;       /*The seed for the ENCR and DECR command*/
	const char *path;        /*The path for the ENCR and DECR command*/
} ParsedArgs;

/**Struct for passing arguments to the thread that handles ENCR and DECR commands*/
typedef struct ThreadArgs {
	ParsedArgs args;
	Socket s;
} ThreadArgs;

static void usage(char *exec_name);
static void parse_args(int argc, char **argv, ParsedArgs *args);
static int strtoipandport(char *hostandport, unsigned long *ip, u_short *port);
static void save_seed(unsigned int seed, const char *path);
static char* str_retcode(int retcode);
static void send_thread_command(void *, void *);

int main(int argc, char **argv) {
	ParsedArgs args;
	parse_args(argc, argv, &args);

	socket_startup();
	Socket sock = connect_socket(args.host_addr, args.host_port);

	int ret_code = 0;
	//encryption and decryption command should run in another thread as per proj. spec.
	if(strcmp(args.cmd, ENCR) == 0 || strcmp(args.cmd, DECR) == 0) {
		Thread t;
		ThreadArgs ta = {args, sock};

		//it is OK to pass stack variables to the thread in this situation because
		//we are going to join right after, mantaining such variables valid.
		thread_create(&t, &send_thread_command, &ta, &ret_code);
		thread_join(&t);
	} else {
		ret_code = cryptor_send_command(sock, args.cmd, args.seed, args.path);
	}

	if(strcmp(args.cmd, ENCR) == 0 && ret_code == RETOK_INT)
		save_seed(args.seed, args.path);

	printf("Server responded with code %d: %s.\n\n", ret_code, str_retcode(ret_code));
	//If the response of the server is 300 read the remaining output
	if(ret_code == RETMORE_INT) {
		cryptor_print_more(sock);
	}

	socket_close(sock);
	socket_cleanup();
}

static void send_thread_command(void *args, void *retval) {
	ThreadArgs *t_args = (ThreadArgs *) args;
	int *ret_code = (int *) retval;
	*ret_code = cryptor_send_command(t_args->s, t_args->args.cmd, t_args->args.seed, t_args->args.path);
}

static void parse_args(int argc, char **argv, ParsedArgs *args) {
	if(argc < 3) usage(argv[0]);

	//get the ip addr and the port
	if(strtoipandport(argv[argc - 1], &args->host_addr, &args->host_port)) {
		usage(argv[0]);
	}
	if(args->host_port == 0) args->host_port = htons(CRYPTOR_DEFAULT_PORT);

	//get the options
	int c;
	args->cmd = NULL;
	args->seed = 0;
	opterr = 0;
	switch ((c = getopt(argc, argv, "lRed"))) {
		case 'l':
		case 'R':
			if(argc != 3) usage(argv[0]);
			args->cmd = c == 'l' ? LSTF : LSTR; //the command
			break;
		case 'e':
		case 'd': {
			if(argc != 5) usage(argv[0]);
			args->cmd = c == 'e' ? ENCR : DECR; //the command
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
	if(args->cmd == NULL) {
		elog("Process directory option missing.");
		usage(argv[0]);
	}
}

static char* str_retcode(int retcode) {
	switch(retcode) {
		case RETOK_INT:
			return "Success";
		case RETERR_INT:
			return "Error: The file doesn't exist or the received command was"
				   " malformed";
		case RETERRTRANS_INT:
			return "Error: The server hasn't been able to encrypt/decrypt the"
				   " file, maybe the file is in use by another process or some"
				   " I/O error occured. Please try again later";
		case RETMORE_INT:
			return "Success, more output incoming";
	}
	return "Unknown return code";
}

/*
 * Parses a string of the form ipaddr:port and returns the ip addr in the "ip" arg
 * and the port in the "port" arg. I no port is specified, then the "port" arg is set to 0
 * @return 0 on success, non 0 on failure and ip and port content is undefined
 */
static int strtoipandport(char *hostandport, unsigned long *ip, u_short *port) {
	char *hoststr = strtok(hostandport, ":");
	if(inet_pton(AF_INET, hoststr, ip) != 1) {
		return -1;
	}

	char *portstr = strtok(NULL, ":");
	if(portstr == NULL) {
		*port = 0;
		return 0;
	}

	char *err;
	long p = strtol(portstr, &err, 10);
	if(*err != '\0' || p < PORT_MIN || p > PORT_MAX) {
		return -1;
	}
	*port = (u_short) htons((uint16_t) p);
	return 0;
}

static void save_seed(unsigned int seed, const char *path) {
	char seedstr[MAX_STRLEN_FOR_INT_TYPE(unsigned int) + 1];
	sprintf(seedstr, "%u", seed);
	FILE *seeds = fopen(SEEDS_FILE, "a");
	fprintf(seeds, "%s %s\n", seedstr, path);
	fclose(seeds);
}

static void usage(char *exec_name) {
	elogf("Usage: %s [-l | -R | -e seed path | -d seed path] ipaddr[:port]\n", exec_name);
	exit(1);
}
