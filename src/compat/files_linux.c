#include "files.h"
#include <stdlib.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

struct Directory {
	DIR *dir;
};

Directory* open_dir(const char *path) {
	if(strlen(path) - 1 > MAX_PATH_LENGTH) {
		puts("Path too long\n");
		return NULL;
	}
	
	Directory *dir = malloc(sizeof(Directory));
	if(!dir) return NULL;
	
	dir->dir = opendir(path);
	if(!dir->dir) {
		free(dir);
		return NULL;
	}
	
	return dir;
}


int close_dir(Directory *dir) {
	int ret = closedir(dir->dir);
	free(dir);
	return ret;
}

int delete_file(const char *path) {
	return unlink(path);
}
