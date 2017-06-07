#ifndef FILE_UTILS_H
#define FILE_UTILS_H
#include "files.h"

void recursive_read(const char *dir_path, void (*process_entry)(const char *, DirInfo *));
void recursive_list(const char *dir_path);

#endif
