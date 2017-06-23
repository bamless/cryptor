#include "files.h"
#include "logging.h"

#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <windows.h>

static void set_err(int *);
static void fix_slash(char *str);

struct Dir {
	HANDLE find;
	WIN32_FIND_DATA ffd;
	int b_first;
};

Dir* open_dir(const char *path, int *err) {
	*err = 0;
	Dir *dir = malloc(sizeof(Dir));
	if(!dir) {
		SetLastError(ERROR_OUTOFMEMORY);
		set_err(err);
		return NULL;
	}

	size_t len = strlen(path) + 7; //+7 for the \\?\, \* and NUL
	char *szDir = malloc(len);
	strcpy(szDir, "\\\\?\\");
	strcat(szDir, path);
	strcat(szDir, "\\*");
	fix_slash(szDir);

	dir->find = FindFirstFile(szDir, &dir->ffd);
	if(dir->find == INVALID_HANDLE_VALUE) {
		set_err(err);
		free(dir);
		return NULL;
	}
	dir->b_first = 1;
	free(szDir);
	return dir;
}

int close_dir(Dir *dir) {
	int ret = FindClose(dir->find);
	free(dir);
	return ret ? 0 : 1;
}

int has_next(Dir *dir) {
	if(dir->b_first) {
		dir->b_first = 0;
		return 1;
	}
	return FindNextFile(dir->find, &dir->ffd) ? 1 : 0;
}

void next_dir(Dir *dir, DirEntry *entry) {
	if(dir->b_first) {
		elog("Error: has_next must be called at least once befor next_dir");
		return;
	}
	memcpy(entry->name, dir->ffd.cFileName, 256);
	if(dir->ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		entry->type = DIRECTORY;
	} else {
		entry->type = NFILE;
	}
}

File open_file(const char *path, int mode, int *err) {
	*err = 0;
	DWORD acc = 0;
	DWORD share = 0;
	if(mode & READ) {
		acc |= GENERIC_READ;
		share |= FILE_SHARE_READ;
	}
	if(mode & WRITE) {
		acc |= GENERIC_WRITE;
		share |= FILE_SHARE_WRITE;
	}
	File f = CreateFile(path, acc, share, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(f == INVALID_HANDLE_VALUE) {
		set_err(err);
		return INVALID_HANDLE_VALUE;
	}
	return f;
}

int close_file(File file) {
	if(!CloseHandle(file)) {
		int err = 0;
		set_err(&err);
		return err;
	}
	return 0;
}

int lock_file(File f, fsize_t off, fsize_t len) {
	if(!LockFile(f, (DWORD) off, (DWORD) (off >> 32), (DWORD) len, (DWORD) (len >> 32))) {
		int err = 0;
		set_err(&err);
		return err;
	}
	return 0;
}

int unlock_file(File f, fsize_t off, fsize_t len) {
	if(!UnlockFile(f, (DWORD) off, (DWORD) (off >> 32), (DWORD) len, (DWORD) (len >> 32))) {
		int err = 0;
		set_err(&err);
		return err;
	}
	return 0;
}

int delete_file(const char *path) {
	if(!DeleteFile((LPCTSTR) path)) {
		int err = 0;
		set_err(&err);
		return err;
	}
	return 0;
}

//Converts slashes to backslashes, needed by some win32 API functions
static void fix_slash(char *str) {
	do {
		if(*str == '/') *str = '\\';
	} while(*(++str));
}

int get_file_size(const char *path, fsize_t *fsize) {
	int err = 0;
	HANDLE hFile = CreateFile(path, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		set_err(&err);
		return err;
	}
	fget_file_size(hFile, fsize);
	CloseHandle(hFile);
	return 0;
}

int fget_file_size(File f, fsize_t *fsize) {
	DWORD dwFileSizeLow = 0;
	DWORD dwFileSizeHigh = 0;
	dwFileSizeLow = GetFileSize(f, &dwFileSizeHigh);
 	*fsize = (((fsize_t) dwFileSizeHigh) << 32) | dwFileSizeLow; //We are guaranteed by the WinAPI that DWORD is always 32 bit
	return 0;
}

int change_dir(const char *path) {
	return SetCurrentDirectory(path) ? 0 : 1;
}

char* get_cwd() {
	int required_len  = GetCurrentDirectory(0, NULL);
	char *cwd = malloc(required_len);
	GetCurrentDirectory(required_len, cwd);
	//covert backslashes to slashes
	char *str = cwd;
	do {
		if(*str == '\\') *str = '/';
	} while(*(++str));
	return cwd;
}

static void set_err(int *err) {
	switch(GetLastError()) {
		case ERROR_FILE_NOT_FOUND:
			*err = ERR_NOFILE;
			break;
		case ERROR_DIRECTORY:
			*err = ERR_NOTDIR;
			break;
		case ERROR_ACCESS_DENIED:
			*err = ERR_ACCESS;
			break;
		case ERROR_MRM_FILEPATH_TOO_LONG:
			*err = ERR_NAMETOOLONG;
			break;
		default:
			*err = ERR_GENERIC;
			break;
	}
}
