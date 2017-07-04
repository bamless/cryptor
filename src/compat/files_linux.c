#include "files.h"
#include "logging.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

static int get_err();

struct Dir {
	DIR *dir;
	struct dirent *prev_entry;
	struct dirent *dir_entry;
};

static void out_of_mem(DIR *unix_dir, int *err) {
	errno = ENOMEM;
	*err = get_err();
	closedir(unix_dir);
}

Dir* open_dir(const char *path, int *err) {
	*err = 0;
	DIR *unix_dir = opendir(path);
	if(!unix_dir) {
		*err = get_err();
		return NULL;
	}

	Dir *dir = malloc(sizeof(Dir));
	if(!dir) {
		out_of_mem(unix_dir, err);
		return NULL;
	}

	int len_entry = offsetof(struct dirent, d_name) + fpathconf(dirfd(unix_dir), _PC_NAME_MAX) + 1;
	dir->dir = unix_dir;
	dir->prev_entry = malloc(len_entry);
	if(!dir->prev_entry) {
		out_of_mem(unix_dir, err);
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
	readdir_r(unix_dir, dir->prev_entry, &dir->dir_entry); //Thread safety, yeee
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
	entry->name[255] = '\0'; //null terminate just in case strncpy truncates
}

File open_file(const char *path, int mode, int *err) {
	int flags = 0; *err = 0;
	if((mode & READ) && (mode & WRITE)) {
		flags |= O_RDWR;
	} else if(mode & READ) {
		flags |= O_RDONLY;
	} else if(mode & WRITE) {
		flags |= O_WRONLY;
	}

	mode_t m = 0;
	if(mode & CREATE) {
		flags |= O_CREAT;
		m = 0744;
	}

	int file = open(path, flags, m);
	if(file < 0) {
		*err = get_err();
		return -1;
	}
	return file;
}

int close_file(File file) {
	if(close(file)) {
		return get_err();
	}
	return 0;
}

int delete_file(const char *path) {
	if(unlink(path)) {
		return get_err();
	}
	return 0;
}

int get_file_size(const char *path, fsize_t *fsize) {
	struct stat file_stat;
	if(stat(path, &file_stat)) {
		return get_err();
	}
	*fsize = file_stat.st_size;
	return 0;
}

int fget_file_size(File f, fsize_t *fsize) {
	struct stat file_stat;
	if(fstat(f, &file_stat)) {
		return get_err();
	}
	*fsize = file_stat.st_size;
	return 0;
}

int lock_file(File f, fsize_t off, fsize_t len) {
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = off;
	lock.l_len = len;
	lock.l_pid = getpid();

 	if(fcntl(f, F_SETLK, &lock)) {
		return get_err();
	}
	return 0;
}

int unlock_file(File f, fsize_t off, fsize_t len) {
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = off;
	lock.l_len = len;
	lock.l_pid = getpid();

 	if(fcntl(f, F_SETLK, &lock)) {
		return get_err();
	}
	return 0;
}

int change_dir(const char *path) {
	return chdir(path);
}

char* get_cwd() {
	int size = 1024;
	char *pwd = malloc(size);
	while(!getcwd(pwd, size) && errno == ERANGE) {
		size *= 2;
		pwd = realloc(pwd, size);
	}
	return pwd;
}

int rename_file(const char *oldpath, const char *newpath) {
	if(rename(oldpath, newpath)) {
		return get_err();
	}
	return 0;
}

char* get_abs(const char *path) {
	char resolved[PATH_MAX];
	if(realpath(path, resolved) == NULL) {
		return NULL;
	}
	char *resolved_dyn = malloc(strlen(resolved) + 1);
	strcpy(resolved_dyn, resolved);
	return resolved_dyn;
}

static int get_err() {
	switch(errno) {
		case ENOENT:
			return ERR_NOFILE;
		case EACCES:
			return ERR_ACCESS;
		case ENOTDIR:
			return ERR_NOTDIR;
		case ENAMETOOLONG:
			return ERR_NAMETOOLONG;
		default:
			return ERR_GENERIC;
	}
}
