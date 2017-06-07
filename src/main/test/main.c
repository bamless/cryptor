#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "files.h"
#include "logging.h"
#include "fileUtils.h"

static void recursive_readz(const char *path);

int main() {
 recursive_list("/home/fabrizio/HDD/Fabrizio/Pictures");
}

static void recursive_readz(const char *path) {
  Dir *d = open_dir(path);
  if(!d) {
    perr("error");
    return;
  }
  DirInfo entry;
  while(has_next(d)) {
    next_dir(d, &entry);
    if(strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0) {
      logf("%s/%s ", path, entry.name);
      if(entry.type == NFILE) logs("NFILE");
      if(entry.type == DIRECTORY) {
        logs("DIR");
        char subdir_path[MAX_PATH_LENGTH];
        snprintf(subdir_path, sizeof(subdir_path)-1, "%s/%s", path, entry.name);
        recursive_readz(subdir_path);
      }
    }
  }
  close_dir(d);
}
