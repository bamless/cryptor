#ifndef FILES_H
#define FILES_H


#ifdef __unix
#include <sys/types.h>
typedef off_t fsize_t;
#endif
#ifdef _WIN32
#include <inttypes.h>
typedef uint64_t fsize_t;
#endif

#define MAX_PATH_LENGTH 512

/**
 * Portable directory API for C. It encapsulates platform-specific directory
 * logic and provides a unified interface. For concrete implementation see
 * files_*plaform*.c
 */
typedef struct Dir Dir; //opaque type

//portable error codes
#define ERR_NOFILE 1
#define ERR_ACCESS 2
#define ERR_NOTDIR 3
#define ERR_NAMETOOLONG 4
#define ERR_GENERIC 5

/*Enum for the different file types a directory can hold*/
typedef enum FileType {
  NFILE, DIRECTORY, UNKNW
} FileType;
/*Struct holding a directory entry info*/
typedef struct DirEntry {
  FileType type;
  char name[256];
} DirEntry;

/*Opens a directory at path "path. If fails it returns NULL and err is set to the appropriate error code"*/
Dir* open_dir(const char *path, int *err);
/*Closes Dir. returns 0 on success, nonzero if it fails*/
int close_dir(Dir *dir);
/*Returns 1 if the Dir object has a next element, 0 otherwise*/
int has_next(Dir *dir);
/*Fills the DirEntry struct with info of next element in Dir*/
void next_dir(Dir *dir, DirEntry *entry);

// End directory API

/*Deletes the file at path. Returns 0 on success, nonzero on failure*/
int delete_file(const char *path);
/*Returns the size of the file at path. the value returned by this funtion in fsize
is guaranteed to be >= 0. The actual type of fsize_t is implementation defined.
@return 0 on success and fsize is set to the file size. non 0 error code on failure*/
int get_file_size(const char *path, fsize_t *fsize);
/*Changes the working directory of the process. Returns 0 on success, non 0 error on failure*/
int change_dir(const char *path);

#endif
