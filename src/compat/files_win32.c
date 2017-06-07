#include "files.h"
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <windows.h>

struct Directory {
	HANDLE find;
};

Directory* open_dir(const char *path) {
	if(strlen(path) - 3 > MAX_PATH_LENGTH) {
		puts("Path too long\n");
		return NULL;
	}
	
	Directory *dir = malloc(sizeof(Directory));
	if(!dir) return NULL;
	
	char szDir[MAX_PATH_LENGTH];

	strcpy(szDir, path);
	strcat(szDir, "\\*");
	
	WIN32_FIND_DATA ffd;
	
	dir->find = FindFirstFile(szDir, &ffd);
	if(dir->find == INVALID_HANDLE_VALUE) {
		free(dir);
		return NULL;
	}
	return dir;
}

int close_dir(Directory *dir) {
	int ret = FindClose(dir->find);
	free(dir);
	if(ret) return 0;
	else return -1;
}

int delete_file(const char *path) {
	if(DeleteFile((LPCTSTR) path)) return 0;
	else return -1;
}
