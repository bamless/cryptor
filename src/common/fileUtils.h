#ifndef NFILE_UTILS_H
#define NFILE_UTILS_H
#include "files.h"

void recursive_explore(const char *dir_path, void (*process_entry)(const char *, DirEntry *));
void recursive_list(const char *dir_path);

#endif
