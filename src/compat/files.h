#ifndef FILES_H
#define FILES_H

#define MAX_PATH_LENGTH 1024

/**
 * Portable directory API for C. It encapsulates platform-specific directory
 * logic and provides a unified interface. For concrete implementation see
 * files_*plaform*.c
 */
typedef struct Dir Dir;

/*Enum for the different file types a directory can hold*/
typedef enum FileType {
  NFILE, DIRECTORY, UNKNW
} FileType;
/*Struct holding a directory entry info*/
typedef struct DirInfo {
  FileType type;
  char name[256];
} DirInfo;

/*Opens a directory at path "path"*/
Dir* open_dir(const char *path);
int close_dir(Dir *dir);
/*Returns 1 if the Dir object has a next element*/
int has_next(Dir *dir);
/*Fills the DirInfo struct with info of next element in Dir*/
void next_dir(Dir *dir, DirInfo *dir_info);

// End directory API

/*Deletes the file at path*/
int delete_file(const char *path);

#endif
