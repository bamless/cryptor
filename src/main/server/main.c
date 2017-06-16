#include "socket.h"
#include "logging.h"
#include "error.h"
#include "threadpool.h"
#include "connhandler.h" /*handle_connection(void * socket)*/

#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>

#define DEFAULT_PORT 8888
#define DEFAULT_THREADS 20

typedef struct Config {
	u_short port;
	char *pwd;
	int thread_count;
} Config;

static void usage(const char *exec_name);
static void init_config(Config *cfg);
static void parse_arg(int argc, char **argv, Config *cfg);
static void init_server_socket(Socket *server_sock, Config *cfg);

int main(int argc, char **argv) {
	Config cfg;
	init_config(&cfg);
	parse_arg(argc, argv, &cfg);

	socket_startup();

	ThreadPool *tp = threadpool_create(cfg.thread_count);
	Socket server_sock, client_sock;
	init_server_socket(&server_sock, &cfg);

	struct sockaddr_in client;
	int client_len = sizeof(client);
	while((client_sock = accept(server_sock, (struct sockaddr *) &client, &client_len))) {
		if(!is_socket_valid(client_sock)) continue;

		Socket *incoming_conn = malloc(sizeof(Socket));
		*incoming_conn = client_sock;
		threadpool_add_task(tp, &handle_connection, incoming_conn);
	}

	threadpool_destroy(tp, SOFT_SHUTDOWN);
	socket_close(server_sock);

	socket_cleanup();
}

static void init_server_socket(Socket *server_sock, Config *cfg) {
	struct sockaddr_in server;

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(cfg->port);

	*server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(!is_socket_valid(*server_sock)) {
		perr_sock("Error: creating socket");
		socket_cleanup();
		exit(1);
	}
	if(bind(*server_sock, (struct sockaddr *) &server, sizeof(server))) {
		perr_sock("Error: bind server");
		socket_cleanup();
		exit(1);
	}
	if(listen(*server_sock, SOMAXCONN)) {
		perr_sock("Error: bind server");
		socket_cleanup();
		exit(1);
	}
}

static void parse_arg(int argc, char **argv, Config *cfg) {
	int c, cflag = 0;
	while((c = getopt(argc, argv, ":c:n:p:")) != -1) {
		switch(c) {
			case 'c':
				cflag++;
				cfg->pwd = optarg;
				break;
			case 'p': {
				char *err;
				long int p = strtol(optarg, &err, 10);
				if(*err != '\0' || p < PORT_MIN || p > PORT_MAX)
					usage(argv[0]);
				cfg->port = (u_short) p;
				break;
			}
			case 'n': {
				char *err;
				long int tc = strtol(optarg, &err, 10);
				if(*err != '\0' || tc < 1 || tc > INT_MAX)
					usage(argv[0]);
				cfg->thread_count = (int) tc;
				break;
			}
			case '?':
				if(isprint(optopt))
	  				elogf("Unknown option `-%c`.\n", optopt);
				else
	  				elogf("Unknown option character `\\x%x`.\n", optopt);
				usage(argv[0]);
				break;
			case ':':
				elogf("No argument to `-%c`.\n", optopt);
				usage(argv[0]);
				break;
		}
	}
	if(!cflag) usage(argv[0]); //-c option is mandatory
}

static void init_config(Config *cfg) {
	cfg->port = DEFAULT_PORT;
	cfg->thread_count = DEFAULT_THREADS;
	cfg->pwd = NULL; //this is a mandatory arg, so we don't need a default value
}

static void usage(const char *exec_name) {
	elogf("Usage: %s [-p port] [-n threads] -c pwd\n", exec_name);
	exit(1);
}
