#include "socket.h"
#include "logging.h"
#include "error.h"
#include "stringbuf.h"
#include "protocol.h"
#include "cryptorclient.h"
#include "thread.h"

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <inttypes.h>

#define SEEDS_FILE "cryptor_seeds"

typedef struct ParsedArgs {
	const char *cmd;			/*The command passed from arguments*/
	unsigned long host_addr;	/*The server address*/
	u_short host_port;			/*The server port*/
	unsigned int seed;			/*The seed for the ENCR and DECR command*/
	const char *path;			/*The path for the ENCR and DECR command*/
} ParsedArgs;

typedef struct ThreadArgs {
	ParsedArgs args;
	Socket s;
} ThreadArgs;

static void usage(char *exec_name);
static void parse_args(int argc, char **argv, ParsedArgs *args);
static void strtoipandport(char *hostandport, unsigned long *ip, u_short *port);
static void save_seed(unsigned int seed, const char *path);
static char* str_retcode(int retcode);
static void send_thread_command(void *, void **);

int main(int argc, char **argv) {
	ParsedArgs args;
	parse_args(argc, argv, &args);

	socket_startup();
	Socket sock = init_connection(args.host_addr, args.host_port);

	int ret_code = 0;
	//encryption and decryption command should run in another thread as per proj. spec.
	if(strcmp(args.cmd, ENCR) == 0 || strcmp(args.cmd, DECR) == 0) {
		int *ret;

		Thread t;
		ThreadArgs *ta = malloc(sizeof(ThreadArgs));
		ta->args = args;
		ta->s = sock;

		thread_create(&t, &send_thread_command, ta, (void **) &ret);
		thread_join(&t);
		ret_code = *ret;

		free(ret);
	} else {
		ret_code = cryptor_send_command(sock, args.cmd, args.seed, args.path);
	}

	if(strcmp(args.cmd, ENCR) == 0 && ret_code == RETOK_INT)
		save_seed(args.seed, args.path);

	printf("Server responded with code %d: %s.\n\n", ret_code, str_retcode(ret_code));
	//If the response of the server is 300 (i.e. the server will send more output) read the remaining output
	if(ret_code == RETMORE_INT) {
		cryptor_print_more(sock);
	}

	socket_close(sock);
	socket_cleanup();
}

static void send_thread_command(void *args, void **retval) {
	int **ret_code = (int **) retval;
	*ret_code = malloc(sizeof(int));
	ThreadArgs *t_args = args;

	**ret_code = cryptor_send_command(t_args->s, t_args->args.cmd, t_args->args.seed, t_args->args.path);

	free(args);
}

static void parse_args(int argc, char **argv, ParsedArgs *args) {
	if(argc < 3) usage(argv[0]);

	//get the ip addr and the port
	strtoipandport(argv[argc - 1], &args->host_addr, &args->host_port);
	if(args->host_addr == INADDR_NONE) usage(argv[0]);
	if(args->host_port == 0) args->host_port = htons(DEFAULT_PORT);

	//get the options
	int c;
	args->cmd = NULL;
	args->seed = 0;
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
	if(args->cmd == NULL) usage(argv[0]);
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
	if(*err != '\0' || p < PORT_MIN || p > PORT_MAX) {
		p = 0;
	}
	*port = (u_short) htons((uint16_t) p);
}

static void save_seed(unsigned int seed, const char *path) {
	char seedstr[11];
	sprintf(seedstr, "%u", seed);
	FILE *seeds = fopen(SEEDS_FILE, "a");
	fprintf(seeds, "%s %s\n", seedstr, path);
	fclose(seeds);
}

static void usage(char *exec_name) {
	elogf("Usage: %s [-l | -R | -e seed path | -d seed path] ipaddr:port\n", exec_name);
	exit(1);
}
