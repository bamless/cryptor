#include "cryptorserver.h"
#include "socket.h"
#include "error.h"
#include "stringbuf.h"
#include "files.h"
#include "logging.h"
#include "protocol.h"
#include "encrypt.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

static void handle_list_commands(Socket client, int is_recursive);
static void handle_encrytion_commands(Socket client, int is_decrypt);

void cryptor_handle_connection(Socket client) {
	char cmd[5];
	memset(cmd, '\0', sizeof(cmd));
	recv(client, cmd, sizeof(cmd) - 1, MSG_WAITALL);

	dlogf("Received command %s\n", cmd);

	if(strcmp(cmd, LSTF) == 0) {
		handle_list_commands(client, 0);
	} else if(strcmp(cmd, LSTR) == 0) {
		handle_list_commands(client, 1);
	} else if(strcmp(cmd, ENCR) == 0){
		handle_encrytion_commands(client, 0);
	} else if(strcmp(cmd, DECR) == 0) {
		handle_encrytion_commands(client, 1);
	} else {
		send(client, RETERR, 3, MSG_NOSIGNAL);
	}

	socket_close(client);
}

static void send_list(Socket s, StringBuffer *path, StringBuffer *cmdline, int is_recursive);

static void handle_list_commands(Socket client, int is_recursive) {
	char *pwd = get_cwd();

	StringBuffer *path = sbuf_create();
	StringBuffer *cmdline = sbuf_create();
	sbuf_appendstr(path, pwd);
	free(pwd);

	send(client, RETMORE, 3, MSG_NOSIGNAL);
	send_list(client, path, cmdline, is_recursive); //sends the directory list
	send(client, "\r\n", 2, MSG_NOSIGNAL); //signal end of output (\r\n\r\n)

	sbuf_destroy(path);
	sbuf_destroy(cmdline);
}

/* Sends the line "fsize file_path\r\n" for every file in the PWD. If is_recursive
 * is non-zero then then it explores recursevely all the subfolders.*/
static void send_list(Socket s, StringBuffer *path, StringBuffer *cmdline, int is_recursive) {
	int err;
	Dir *dir = open_dir(sbuf_get_backing_buf(path), &err);
	if(!dir || err) {
		elogf("Error send_list: %s ", sbuf_get_backing_buf(path));
		perr("open_dir failed");
		return;
	}

	DirEntry entry;
	while(has_next(dir)) {
		next_dir(dir, &entry);
		if(strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;

		int orig_len = sbuf_get_len(path); //save the prev length of the path

		//append the entry name to obtain the full path
		sbuf_appendstr(path, "/");
		sbuf_appendstr(path, entry.name);

		if(entry.type == NFILE) {
			fsize_t fsize;
			if(get_file_size(sbuf_get_backing_buf(path), &fsize)) {
				perr("Error send_list: get_file_size failed");
				sbuf_truncate(path, orig_len);
				continue;
			}

			sbuf_clear(cmdline);

			//+2 for NUL and spaces
			char fsizestr[MAX_STRLEN_FOR_INT_TYPE(uintmax_t) + 2];
			//we can safely cast to uintmax_t because get_file_size guarantees a result >= 0
			sprintf(fsizestr, "%"PRIu64" ", (uintmax_t) fsize);

			//append the cmdline to the sbuf
			sbuf_appendstr(cmdline, fsizestr);
			sbuf_appendstr(cmdline, sbuf_get_backing_buf(path));
			sbuf_appendstr(cmdline, "\r\n");

			if(send(s, sbuf_get_backing_buf(cmdline), sbuf_get_len(cmdline), MSG_NOSIGNAL) < 0) {
				sbuf_truncate(path, orig_len);
				break;
			}
		}
		//if directory and LSTR, secursively explore
		if(entry.type == DIRECTORY && is_recursive) {
			send_list(s, path, cmdline, is_recursive);
		}

		sbuf_truncate(path, orig_len); //remove the entry name from the path
	}
	close_dir(dir);
}

static void receive_command_line(Socket client, StringBuffer *sb) ;
static int parse_encryption_cmdline(StringBuffer *client, unsigned int *seed, char **path);
static char* get_out_name(const char *name, int is_decrypt);
static int strendswith(const char *str, const char *substr);

static void handle_encrytion_commands(Socket client, int is_decrypt) {
	char *path = NULL, *out = NULL;
	File file = NULL_FILE;
	fsize_t s = 0;

	//receive command line from client
	StringBuffer *cmdline = sbuf_create();
	receive_command_line(client, cmdline);

	unsigned int seed;
	if(parse_encryption_cmdline(cmdline, &seed, &path) || (is_decrypt && !strendswith(path, "_enc"))) {
		send(client, RETERR, 3, MSG_NOSIGNAL);
		goto error;
	}

	int err;
	file = open_file(path, READ | WRITE, &err); //WRITE is needed for acquiring an exclusive lock
	if(err) {
		send(client, (err == ERR_NOFILE) ? RETERR : RETERRTRANS, 3, MSG_NOSIGNAL);
		goto error;
	}

	if(fget_file_size(file, &s) || lock_file(file, 0, s)) {
		send(client, RETERRTRANS, 3, MSG_NOSIGNAL);
		goto error;
	}

	out = get_out_name(path, is_decrypt);
	if(encrypt(file, out, seed)) {
		send(client, RETERRTRANS, 3, MSG_NOSIGNAL);
		goto error;
	}

	if(delete_file(path))
		perr("Error deleting plaintext file");

	send(client, RETOK, 3, MSG_NOSIGNAL);

error:
	sbuf_destroy(cmdline);
	unlock_file(file, 0, s);
	close_file(file);
	free(path);
	free(out);
}

static void receive_command_line(Socket client, StringBuffer *sb) {
	char buff[1024];
	ssize_t bytes_recv;
	while((bytes_recv = recv(client, buff, sizeof(buff), 0)) > 0) {
		sbuf_append(sb, buff, bytes_recv);
		if(sbuf_endswith(sb, "\r\n")) break; //\r\n signals end of encr/decr command lines
	}
	if(bytes_recv < 0) perr_sock("Error");
}

static int parse_encryption_cmdline(StringBuffer *cmdline, unsigned int *seed, char **path) {
	sbuf_truncate(cmdline, sbuf_get_len(cmdline) - 2); //remove the \r\n
	size_t cmdline_len = sbuf_get_len(cmdline);

	char *saveptr, *err;
	char *seed_str = strtok_r(sbuf_get_backing_buf(cmdline), " ", &saveptr);

	unsigned long seedl = strtoul(seed_str, &err, 10);
	if(seedl > UINT_MAX || *err) {
		return - 1;
	}
	*seed = (unsigned int) seedl;
	dlogf("seed: %d\n", *seed);

	char *tok;
	*path = malloc(cmdline_len - (strlen(seed_str) + 1) + 1);
	strcpy(*path, strtok_r(NULL, " ", &saveptr));
	while((tok = strtok_r(NULL, " ", &saveptr))) {
		strcat(*path, " ");
		strcat(*path, tok);
	}

	dlogf("file: %s\n", *path);
	return 0;
}

static int strendswith(const char *str, const char *substr) {
	size_t str_len = strlen(str), substr_len = strlen(substr);
	if(str_len < substr_len) return 0;
	return strcmp(str + (str_len - substr_len), substr) == 0;
}

static char* get_out_name(const char *name, int is_decrypt) {
	char *out;
	if(is_decrypt) {
		int len = strlen(name) - 3;
		out = malloc(len);
		strncpy(out, name, len - 1);
		out[len - 1] = '\0';
	} else {
		out = malloc(strlen(name) + 5);
		strcpy(out, name);
		strcat(out, "_enc");
	}
	return out;
}
