#ifndef FILES_H
#define FILES_H

#define MAX_PATH_LENGTH 256

//opaque type
typedef struct Directory Directory;

Directory* open_dir(const char *path);

int close_dir(Directory *dir);

int delete_file(const char *path);

#endif
