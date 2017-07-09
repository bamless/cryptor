#include "socket.h"
#include "logging.h"
#include "error.h"
#include "threadpool.h"
#include "files.h"
#include "socket_utils.h"
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
	char *conf_file;     /*The configuration file path*/
	u_short port;        /*The server port*/
	char *pwd;           /*The process working directory*/
	int thread_count;    /*The number of threads of the threadpool*/
} Config;

/*Linux specific functions for reloading the confs on SIGHUP and for daemonizing*/
#ifdef __unix
#include <signal.h>
static int reload_requested = 0; //flags that indicates that the user wants to reload the config file
static int shut_down = 0;
static void signal_handler(int signal) {
	dlogf("caught signal %d\n", signal);
	if(signal == SIGHUP)
		reload_requested = 1;
	if(signal == SIGTERM)
		shut_down = 1;
}

static void reload_cfg(Config *oldcfg, Socket *server_sock, ThreadPool **tp);
static void daemonize();
#endif

static void usage(const char *exec_name);
static void init_config(Config *cfg);
static void free_config(Config *cfg);
static void parse_args_and_cfg(int argc, char **argv, Config *cfg);
static void threadpool_handle_connection(void *incoming_conn);

int main(int argc, char **argv) {
	Config cfg;
	init_config(&cfg);
	parse_args_and_cfg(argc, argv, &cfg);

	if(change_dir(cfg.pwd)) {
		perr("Error changing dir");
		exit(1);
	}

#ifdef __unix
 	daemonize();

	struct sigaction sa;
	sa.sa_flags = 0; //we want SIGHUP to interrupt the accept syscall, so no SA_RESTART flag
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &signal_handler;
	if(sigaction(SIGHUP, &sa, NULL) || sigaction(SIGTERM, &sa, NULL)) {
		perr("Error signals");
		exit(1);
	}
#endif

	socket_startup();
	Socket server_sock, client_sock;
	server_sock = init_server_socket(htons(cfg.port));
	ThreadPool *tp = threadpool_create(cfg.thread_count);

	struct sockaddr_in client;
	socklen_t client_len = sizeof(client);
	while((client_sock = accept(server_sock, (struct sockaddr *) &client, &client_len))) {
#ifdef __unix
		if(shut_down) break;
		if(reload_requested) reload_cfg(&cfg, &server_sock, &tp);
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

static void threadpool_handle_connection(void *incoming_conn) {
	Socket client = *((Socket *) incoming_conn);
	free(incoming_conn);
	cryptor_handle_connection(client);
}

static void init_config(Config *cfg) {
	cfg->port = DEFAULT_PORT;
	cfg->thread_count = DEFAULT_THREADS;
	cfg->conf_file = NULL;
	cfg->pwd = NULL; //this is a mandatory arg, so we don't need a default value
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
				cfg->conf_file = get_abs(optarg); //retrieve the absolute path of the config file
				if(!cfg->conf_file) {
					perr("Config file");
					usage(argv[0]);
				}
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
	if(cfg->conf_file != NULL)
		read_cfg_file(cfg);
	//If this is NULL it means both the config file and the terminal args didn't specify a directory
	if(cfg->pwd == NULL) usage(argv[0]); //the -c option is mandatory.
}

static void read_cfg_file(Config *cfg) {
	FILE *file = fopen(cfg->conf_file, "r");
	if(file == NULL) {
		perr("Config file");
		return;
	}
	logsf("Reading config file %s\n", cfg->conf_file);

	char line[1024];
	while(fgets(line, 1024, file)) {
		if(strcmp(line, "\n") == 0) continue;

		char *opt = strtok(line, " ");
		char *optarg = strtok(NULL, " ");
		if(optarg == NULL) {
			elogf("Argument missing for option `%s`\n", opt);
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
				strncat(cfg->pwd, " ", 1024);
				strncat(cfg->pwd, remaining, 1024);
			}
			size_t len = strlen(cfg->pwd);
			if(cfg->pwd[len - 1] == '\n') cfg->pwd[len - 1] = '\0'; //remove newline if present
		} else {
			elogf("Unknown conf file option `%s`\n", opt);
		}
	}
	fclose(file);
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

static void free_config(Config *cfg) {
	if(cfg->conf_file) free(cfg->conf_file);
	if(cfg->pwd) free(cfg->pwd);
}

static void usage(const char *exec_name) {
	elogf("Usage: %s [-p port] [-n threads] [-f conffile] -c pwd\n", exec_name);
	exit(1);
}

/* Linux specific stuff */

#ifdef __unix
static void reload_cfg(Config *oldcfg, Socket *server_sock, ThreadPool **tp) {
	//the program doesn't have a conf file
	if(!oldcfg->conf_file) return;

	Config newcfg;
	init_config(&newcfg);
	newcfg.conf_file = strdup(oldcfg->conf_file);
	newcfg.pwd = strdup(oldcfg->pwd);

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
	if(newcfg.thread_count != oldcfg->thread_count) {
		threadpool_destroy(*tp, HARD_SHUTDOWN);
		*tp = threadpool_create(newcfg.thread_count);
	}

	free_config(oldcfg);
	*oldcfg = newcfg;
	reload_requested = 0;
}

static void daemonize() {
	//fork the first time
	pid_t pid = fork();
	if(pid < 0) {
		perr("Error first fork");
		exit(1);
	}
	//exit the parent, now we are detached from the terminal
	if(pid > 0) exit(0);

	//become  a session leader, we are guaranteed from the previous fork that
	//we are not PG leaders, so this shouldn't fail
	pid_t sid = setsid();
	if(sid < 0) {
		perr("Error setsid");
		exit(1);
	}

	signal(SIGHUP, SIG_IGN); //ignore SIGHUP sent by sess. leader on exit
	//fork a second time, so we are not session leaders anymore
	pid = fork();
	if(pid < 0) {
		perr("Error second fork");
		exit(1);
	}
	if(pid > 0) exit(0); //let the session leader exit, now we can't ever re-aquire a controlling terminal

	umask(0); //reset the umask for full control on created files

	//close stdin, stdout and stderr and reopen them on /dev/null
	//we can open some log file as stdout/err, or maybe us syslogd,
	//but for now close them.
	int nullfd = open("/dev/null", O_RDWR);
	dup2(nullfd, 0);
	dup2(nullfd, 1);
	dup2(nullfd, 2);
	close(nullfd);
}
#endif
