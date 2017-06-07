#include "files.h"
#include "logging.h"

#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <windows.h>

struct Dir{
	HANDLE find;
	WIN32_FIND_DATA ffd;
};

Dir* open_dir(const char *path) {
	if(strlen(path) - 3 > MAX_PATH_LENGTH) {
		elog("Error open_dir: Path too long");
		return NULL;
	}

	char szDir[MAX_PATH_LENGTH];

	strcpy(szDir, path);
	strcat(szDir, "\\*");

	WIN32_FIND_DATA ffd;
	HANDLE handle_find = FindFirstFile(szDir, &ffd);
	if(handle_find == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	Dir *dir = malloc(sizeof(Directory));
	if(!dir) {
		SetLastError(ERROR_OUTOFMEMORY);
		FindClose(handle_find);
		return NULL;
	}

	dir->find = handle_find;
	dir->ffd = fdd;
	return dir;
}

int close_dir(Dir *dir) {
	int ret = FindClose(dir->find);
	free(dir);
	return ret ? 0 : GetLastError();
}

int delete_file(const char *path) {
	return DeleteFile((LPCTSTR) path)) ? 0 : GetLastError();
}
