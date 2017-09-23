#include "cryptorclient.h"
#include "socket.h"
#include "error.h"
#include "protocol.h"
#include "logging.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX(a,b) (((a)>(b))?(a):(b))

static void close_and_exit(Socket sock);
static int read_response(Socket sock);

int cryptor_send_command(Socket sock, const char *cmd, unsigned int seed, const char *path) {
	if(strcmp(cmd, LSTF) == 0 || strcmp(cmd, LSTR) == 0) {
		if(send(sock, cmd, 4, 0) < 0) {
			perr_sock("Error send_command");
			close_and_exit(sock);
		}
	} else if(strcmp(cmd, ENCR) == 0 || strcmp(cmd, DECR) == 0) {
		StringBuffer *cmdline = sbuf_create();
		char seedstr[11];
		snprintf(seedstr, sizeof(seedstr), "%u", seed);

		sbuf_appendstr(cmdline, cmd);
		sbuf_appendstr(cmdline, " ");
		sbuf_appendstr(cmdline, seedstr);
		sbuf_appendstr(cmdline, " ");
		sbuf_appendstr(cmdline, path);
		sbuf_appendstr(cmdline, "\r\n");

		if(send(sock, sbuf_get_backing_buf(cmdline), sbuf_get_len(cmdline), 0) < 0) {
			perr_sock("Error send_command");
			close_and_exit(sock);
		}
		sbuf_destroy(cmdline);
	} else {
		elog("Error cryptor_send_command: unknown command");
		close_and_exit(sock);
	}

	return read_response(sock);
}

/*
 * Returns further server output in the StringBuffer sb
 */
void cryptor_read_more(Socket server, StringBuffer *sb) {
	char buff[512];
	ssize_t bytes_recv;
	while((bytes_recv = recv(server, buff, sizeof(buff), 0)) > 0) {
		sbuf_append(sb, buff, bytes_recv);
		if(sbuf_endswith(sb, "\r\n\r\n")) break; //\r\n\r\n signals the end of the output as per protocol spec.
	}
	if(bytes_recv <= 0) perr_sock("Error");
}

/*
 * Prints further server output to stdout
 */
void cryptor_print_more(Socket server) {
	char buff[512 + 1];
	//the last 4 bytes received from the server (used to test \r\n\r\n for output end)
	char last[5];
	memset(last, '\0', 5);

	ssize_t bytes_recv;
	while((bytes_recv = recv(server, buff, sizeof(buff) - 1, 0)) > 0) {
		//print the received output to stdout
		buff[bytes_recv] = '\0';
		printf("%s", buff);

		//append the last bytes received to last while shifting to the left
		int shift = 4 - MAX(4 - bytes_recv, 0);
		memmove(last , last + shift, 4 - shift);
		memcpy(last + (4 - shift), buff + bytes_recv - shift, shift);

		if(strcmp(last, "\r\n\r\n") == 0) {
			dlog("End of output");
			break; //\r\n\r\n signals the end of the output as per protocol spec.
		}
	}
	if(bytes_recv < 0) perr_sock("Error");
}

static int read_response(Socket sock) {
	//read response code
	char resp[4];
	int received;
	memset(resp, '\0', sizeof(resp));

	if((received = recv(sock, resp, sizeof(resp) - 1, MSG_WAITALL)) == -1) {
		perr_sock("Error read_response");
		close_and_exit(sock);
	}
	if(received == 0) {
		elog("Socket closed by foreign host");
		close_and_exit(sock);
	}

	return (int) strtol(resp, NULL, 0);
}

static void close_and_exit(Socket sock) {
	socket_close(sock);
	socket_cleanup();
	exit(1);
}
