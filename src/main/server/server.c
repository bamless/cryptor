#include "socket.h"
#include "logging.h"
#include "error.h"
#include "threadpool.h"
#include "files.h"

//cyptor protocol headers
#include "protocol.h"
#include "cryptorserver.h" /*cryptor_handle_connection(Socket socket)*/

#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>

#define DEFAULT_THREADS 20

typedef struct Config {
	u_short port;		/*The server port*/
	char *pwd;			/*The process working directory*/
	int thread_count;	/*The number of threads of the threadpool*/
} Config;

static void usage(const char *exec_name);
static void init_config(Config *cfg);
static void parse_args(int argc, char **argv, Config *cfg);
static void threadpool_handle_connection(void *incoming_conn, int id);

int main(int argc, char **argv) {
	Config cfg;
	init_config(&cfg);
	parse_args(argc, argv, &cfg);

	if(change_dir(cfg.pwd)) {
		perr("Error");
		exit(1);
	}

	socket_startup();

	ThreadPool *tp = threadpool_create(cfg.thread_count);
	Socket server_sock, client_sock;
	server_sock = init_server_socket(htons(cfg.port));

	struct sockaddr_in client;
	socklen_t client_len = sizeof(client);
	while((client_sock = accept(server_sock, (struct sockaddr *) &client, &client_len))) {
		if(!is_socket_valid(client_sock)) continue;

		Socket *incoming_conn = malloc(sizeof(Socket));
		*incoming_conn = client_sock;
		threadpool_add_task(tp, &threadpool_handle_connection, incoming_conn);
	}

	threadpool_destroy(tp, SOFT_SHUTDOWN);
	socket_close(server_sock);

	socket_cleanup();
}

static void threadpool_handle_connection(void *incoming_conn, int id) {
	logsf("Thread %d is handling connection\n", id);
	Socket client = *((Socket *) incoming_conn);
	free(incoming_conn);
	cryptor_handle_connection(client);
	logsf("Thread %d done\n", id);
}

static void parse_args(int argc, char **argv, Config *cfg) {
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
