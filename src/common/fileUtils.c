#include "fileUtils.h"
#include "files.h"
#include "error.h"

#include <stdio.h>
#include <string.h>

void recursive_explore(const char *dir_path, void (*process_entry)(const char *, DirEntry *)) {
    Dir *dir = open_dir(dir_path);
    if(!dir) {
        perr("Error: recursive_explore");
        return;
    }
    DirEntry entry;
    while(has_next(dir)) {
        next_dir(dir, &entry);
        if(strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0) {
            process_entry(dir_path, &entry);
            if(entry.type == DIRECTORY) {
                char subdir_path[MAX_PATH_LENGTH];
                snprintf(subdir_path, sizeof(subdir_path)-1, "%s/%s", dir_path, entry.name);
                recursive_explore(subdir_path, process_entry);
            }
        }
    }
    close_dir(dir);
}

static void print_entry(const char *curr_path, DirEntry *entry) {
    printf("%s/%s\n", curr_path, entry->name);
}

void recursive_list(const char *dir_path) {
    recursive_explore(dir_path, &print_entry);
}
