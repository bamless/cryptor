#include "files.h"
#include "logging.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

struct Dir {
	DIR *dir;
	struct dirent *prev_entry;
	struct dirent *dir_entry;
};

Dir* open_dir(const char *path) {
	if(!(strlen(path) < 1) && strlen(path) - 1 > MAX_PATH_LENGTH) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	DIR *unix_dir = opendir(path);
	if(unix_dir) return NULL;

	Dir *dir = malloc(sizeof(Dir));
	if(!dir) {
		errno = ENOMEM;
		closedir(unix_dir);
		return NULL;
	}

	int len_entry = offsetof(struct dirent, d_name) + fpathconf(dirfd(unix_dir), _PC_NAME_MAX) + 1;
	dir->dir = unix_dir;
	dir->prev_entry = malloc(len_entry);
	if(!dir->prev_entry) {
		errno = ENOMEM;
		closedir(unix_dir);
		return NULL;
	}
	dir->dir_entry = NULL;
	return dir;
}


int close_dir(Dir *dir) {
	int ret = closedir(dir->dir);
	free(dir->prev_entry);
	free(dir);
	return ret;
}

int has_next(Dir *dir) {
	DIR *unix_dir = dir->dir;
	readdir_r(unix_dir, dir->prev_entry, &dir->dir_entry);
	return dir->dir_entry ? 1 : 0;
}

void next_dir(Dir *dir, DirEntry *entry) {
	if(dir->dir_entry == NULL) {
		elog("Error: has_next must be called at least once befor next_dir");
		return;
	}

	struct dirent *unix_dirent = dir->dir_entry;
	switch (unix_dirent->d_type) {
		case DT_REG:
			entry->type = NFILE;
			break;
		case DT_DIR:
			entry->type = DIRECTORY;
			break;
		default:
			entry->type = UNKNW;
			break;
	}
	strncpy(entry->name, unix_dirent->d_name, 255);
}

int delete_file(const char *path) {
	return unlink(path);
}
