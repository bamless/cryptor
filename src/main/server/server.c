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
#define DEFAULT_CFG_FILE "cryptor.conf" //if no configuration file is provided, search for this default in the process initial PWD

typedef struct Config {
	char *conf_file;	/*The configuration file path*/
	u_short port;		/*The server port*/
	char *pwd;			/*The process working directory*/
	int thread_count;	/*The number of threads of the threadpool*/
} Config;

#ifdef __unix
#include <signal.h>
static int reload_requested = 0; //flags that indicates that the user wants to reload the config file
static void signal_handler(int signal) {
	dlog("catched SIGHUP");
	reload_requested = 1; //sighup is going to set the flag
}
static void reload_cfg(Config *oldcfg, Socket *server_sock);
#endif

static void usage(const char *exec_name);
static void init_config(Config *cfg);
static void free_config(Config *cfg);
static void parse_args_and_cfg(int argc, char **argv, Config *cfg);
static void threadpool_handle_connection(void *incoming_conn, int id);

int main(int argc, char **argv) {

#ifdef __unix
	struct sigaction sa;
	sa.sa_flags = 0; //we want SIGHUP to interrupt the accept syscall, so no SA_RESTART flag
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &signal_handler;
	if(sigaction(SIGHUP, &sa, NULL) == -1) {
		perr("Error");
		exit(1);
	}
#endif

	Config cfg;
	init_config(&cfg);
	parse_args_and_cfg(argc, argv, &cfg);

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
#ifdef __unix
		if(reload_requested) {
			dlog("reloading configs...");
			reload_cfg(&cfg, &server_sock);
		}
#endif
		if(!is_socket_valid(client_sock)) continue;

		Socket *incoming_conn = malloc(sizeof(Socket));
		*incoming_conn = client_sock;
		threadpool_add_task(tp, &threadpool_handle_connection, incoming_conn);
	}

	threadpool_destroy(tp, SOFT_SHUTDOWN);
	socket_close(server_sock);

	socket_cleanup();
	free_config(&cfg);
}

static void threadpool_handle_connection(void *incoming_conn, int id) {
	dlogf("Thread %d is handling connection\n", id);
	Socket client = *((Socket *) incoming_conn);
	free(incoming_conn);
	cryptor_handle_connection(client);
	dlogf("Thread %d done\n", id);
}

static void read_cfg_file(Config *cfg);
static u_short parse_port(const char *portstr);
static int parse_numthreads(const char *numthreads_str);

static void parse_args_and_cfg(int argc, char **argv, Config *cfg) {
	int c = 0;
	while((c = getopt(argc, argv, ":c:n:p:f:")) != -1) {
		switch(c) {
			case 'c':
				cfg->pwd = strdup(optarg); //we may need to modify the string later, so strdup it
				break;
			case 'p':
				cfg->port = parse_port(optarg);
				if(cfg->port == 0) usage(argv[0]);
				break;
			case 'n':
				cfg->thread_count = parse_numthreads(optarg);
				if(cfg->thread_count == 0) usage(argv[0]);
				break;
			case 'f':
				free(cfg->conf_file);
				cfg->conf_file = get_abs(optarg); //retrieve the absolute path of the config file
				break;
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
	read_cfg_file(cfg);
	//If this is NULL it means both the config file and the terminal args didn't specify a directory
	if(cfg->pwd == NULL) usage(argv[0]); //the -c option is mandatory.
}

static void read_cfg_file(Config *cfg) {
	FILE *file = fopen(cfg->conf_file, "r");
	if(file == NULL) {
		elog("No config file found");
		return;
	}
	logsf("Reading config file %s\n", cfg->conf_file);

	char line[1024];
	while(fgets(line, 1024, file)) {
		char *opt = strtok(line, " ");
		char *optarg = strtok(NULL, " ");
		if(optarg == NULL) {
			elogf("No argument to option `%s`.", opt);
			return;
		}

		if(strcmp(opt, "threads") == 0) {
			cfg->thread_count = parse_numthreads(optarg);
			if(cfg->thread_count == 0) {
				elog("Option `threads` of conf file is malformed.");
				return;
			}
		} else if(strcmp(opt, "port") == 0) {
			cfg->port = parse_port(optarg);
			if(cfg->port == 0) {
				elog("Option `port` of conf file is malformed.");
				return;
			}
		} else if(strcmp(opt, "directory") == 0) {
			if(cfg->pwd) free(cfg->pwd);
			cfg->pwd = malloc(1024);
			strncpy(cfg->pwd, optarg, 1024);
			char *remaining;
			while((remaining = strtok(NULL, " "))) {
				strcat(cfg->pwd, " ");
				strcat(cfg->pwd, remaining);
			}
			cfg->pwd[strlen(cfg->pwd) - 1] = '\0';
		}
	}
}

static u_short parse_port(const char *portstr) {
	char *err;
	long int p = strtol(portstr, &err, 10);
	if((*err != '\0' && *err != '\n') || p < PORT_MIN || p > PORT_MAX)
		return 0;
	return (u_short) p;
}

static int parse_numthreads(const char *numthreads_str) {
	char *err;
	long int tc = strtol(numthreads_str, &err, 10);
	if((*err != '\0' && *err != '\n') || tc < 1 || tc > INT_MAX)
		return 0;
	return (int) tc;
}

static void init_config(Config *cfg) {
	cfg->conf_file = get_abs(DEFAULT_CFG_FILE);
	cfg->port = DEFAULT_PORT;
	cfg->thread_count = DEFAULT_THREADS;
	cfg->pwd = NULL; //this is a mandatory arg, so we don't need a default value
}

static void free_config(Config *cfg) {
	free(cfg->conf_file);
	if(cfg->pwd) free(cfg->pwd);
}

#ifdef __unix
static void reload_cfg(Config *oldcfg, Socket *server_sock) {
	Config newcfg;
	init_config(&newcfg);

	newcfg.conf_file = strdup(oldcfg->conf_file);
	read_cfg_file(&newcfg);

	if(strcmp(newcfg.pwd, oldcfg->pwd) != 0) {
		printf("%s\n", newcfg.pwd);
		if(change_dir(newcfg.pwd)) {
			perr("Error chdir");
			exit(1);
		}
	}
	if(newcfg.port != oldcfg->port) {
		socket_close(*server_sock);
		*server_sock = init_server_socket(htons(newcfg.port));
	}
	free_config(oldcfg);
	*oldcfg = newcfg;
	reload_requested = 0;
}
#endif

static void usage(const char *exec_name) {
	elogf("Usage: %s [-p port] [-n threads] -c pwd\n", exec_name);
	exit(1);
}
