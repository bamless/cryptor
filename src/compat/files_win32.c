#include "files.h"
#include "logging.h"

#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <windows.h>

static void fix_slash(char *str, int length);

struct Dir {
	HANDLE find;
	WIN32_FIND_DATAW ffd;
	int b_first;
};

Dir* open_dir(const char *path) {
	if(!(strlen(path) < 7) && strlen(path) - 7 > MAX_PATH_LENGTH) {
		elog("Error open_dir: Path too long");
		return NULL;
	}

	char szDir[MAX_PATH_LENGTH];
	strcpy(szDir, "\\\\?\\");
	strcat(szDir, path);
	strcat(szDir, "\\*");
	fix_slash(szDir, MAX_PATH_LENGTH);

	WCHAR wSzDir[MAX_PATH_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, szDir, MAX_PATH_LENGTH, wSzDir, MAX_PATH_LENGTH);

	WIN32_FIND_DATAW ffd;
	HANDLE handle_find = FindFirstFileW(wSzDir, &ffd);
	if(handle_find == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	Dir *dir = malloc(sizeof(Dir));
	if(!dir) {
		SetLastError(ERROR_OUTOFMEMORY);
		FindClose(handle_find);
		return NULL;
	}

	dir->find = handle_find;
	dir->ffd = ffd;
	dir->b_first = 1;
	return dir;
}

int close_dir(Dir *dir) {
	int ret = FindClose(dir->find);
	free(dir);
	return ret ? 0 : GetLastError();
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
	return DeleteFile((LPCTSTR) path) ? 0 : GetLastError();
}

static void fix_slash(char *str, int length) {
	for(int i = 0; i < length; i++) {
		if(str[i] == '\0') break;
		if(str[i] == '/') str[i] = '\\';
	}
}
