#include "socket.h"
#include "threadpool.h"
#include "logging.h"
#include "utilsCompat.h"
#include "error.h"
#include "files.h"
#include "stringbuf.h"
#include "fileUtils.h"

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

int main() {
	StringBuffer *sb = sbuf_create();
	sbuf_appendstr(sb, "HeyTest");
	logf("%d\n", sbuf_endswith(sb, "Test"));
	int err;
	Dir *dir = open_dir("D:/Fabrizio\\Musica", &err);
	if(!dir) {
		perr("Error: recursive_explore");
	}
	DirEntry entry;
	while(has_next(dir)) {
		next_dir(dir, &entry);
		if(strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0) {
			logs(entry.name);
		}
	}
	close_dir(dir);
}
