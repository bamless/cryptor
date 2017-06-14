#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void perr(const char *msg) {
	perror(msg);
}

void perr_sock(const char *msg) {
	perror(msg);
}

void perr_exit(const char *msg) {
	perror(msg);
	exit(errno);
}
