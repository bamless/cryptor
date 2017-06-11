#include "files.h"
#include "logging.h"

#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <windows.h>

static void set_err(int *);
static void fix_slash(char *str, int length);

struct Dir {
	HANDLE find;
	WIN32_FIND_DATAW ffd;
	int b_first;
};

Dir* open_dir(const char *path, int *err) {
	if(!(strlen(path) < 7) && strlen(path) - 7 > MAX_PATH_LENGTH) {
		SetLastError(ERROR_MRM_FILEPATH_TOO_LONG);
		set_err(err);
		return NULL;
	}

	Dir *dir = malloc(sizeof(Dir));
	if(!dir) {
		SetLastError(ERROR_OUTOFMEMORY);
		set_err(err);
		return NULL;
	}

	char szDir[MAX_PATH_LENGTH];
	strcpy(szDir, "\\\\?\\");
	strcat(szDir, path);
	strcat(szDir, "\\*");
	fix_slash(szDir, MAX_PATH_LENGTH);

	WCHAR wSzDir[MAX_PATH_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, szDir, MAX_PATH_LENGTH, wSzDir, MAX_PATH_LENGTH);

	dir->find = FindFirstFileW(wSzDir, &dir->ffd);
	if(dir->find == INVALID_HANDLE_VALUE) {
		set_err(err);
		free(dir);
		return NULL;
	}
	dir->b_first = 1;
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
	} else if (dir->ffd.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) {
		entry->type = NFILE;
	} else {
		entry->type = UNKNW;
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

static void fix_slash(char *str, int length) {
	for(int i = 0; i < length; i++) {
		if(str[i] == '\0') break;
		if(str[i] == '/') str[i] = '\\';
	}
}

fsize_t get_file_size(const char *path, int *err) {
	DWORD dwFileSizeLow;
	DWORD dwFileSizeHigh;
	HANDLE hFile = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		set_err(err);
		return 0;
	}

	dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
 	fsize_t fullSize = (((fsize_t) dwFileSizeHigh) << 32) | dwFileSizeLow;
	return fullSize;
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
		default:
			*err = ERR_GENERIC;
			break;
	}
}
