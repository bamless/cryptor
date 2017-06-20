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
	WIN32_FIND_DATAW ffd;
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

	WCHAR *wSzDir = malloc(sizeof(WCHAR) * len);
	MultiByteToWideChar(CP_UTF8, 0, szDir, len, wSzDir, len);

	dir->find = FindFirstFileW(wSzDir, &dir->ffd);
	if(dir->find == INVALID_HANDLE_VALUE) {
		set_err(err);
		free(dir);
		return NULL;
	}
	dir->b_first = 1;
	free(szDir);
	free(wSzDir);
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
	return FindNextFileW(dir->find, &dir->ffd) ? 1 : 0;
}

void next_dir(Dir *dir, DirEntry *entry) {
	if(dir->b_first) {
		elog("Error: has_next must be called at least once befor next_dir");
		return;
	}
	WideCharToMultiByte(CP_UTF8, 0, dir->ffd.cFileName, 256, entry->name, 256, NULL, NULL);
	if(dir->ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		entry->type = DIRECTORY;
	} else {
		entry->type = NFILE;
	}
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
	DWORD dwFileSizeLow = 0;
	DWORD dwFileSizeHigh = 0;
	HANDLE hFile = CreateFile(path, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		set_err(&err);
		return err;
	}

	dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
	CloseHandle(hFile);
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
