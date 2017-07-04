#ifndef FILES_H
#define FILES_H


#ifdef __unix
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
typedef off_t fsize_t;
#elif _WIN32
#include <inttypes.h>
#include <windows.h>
typedef uint64_t fsize_t;
#endif

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

/*Opens a directory at path "path. If it fails returns NULL and err is set to the appropriate error code"*/
Dir* open_dir(const char *path, int *err);
/*Closes Dir. returns 0 on success, nonzero if it fails*/
int close_dir(Dir *dir);
/*Returns 1 if the Dir object has a next element, 0 otherwise*/
int has_next(Dir *dir);
/*Fills the DirEntry struct with info of next element in Dir*/
void next_dir(Dir *dir, DirEntry *entry);

// End directory API

#ifdef __unix
typedef int File;
#elif _WIN32
typedef HANDLE File;
#endif

#define READ 1
#define WRITE 2
#define CREATE 4

/*Opens a file at path path. If the file is not found or an error occurs
 *err is nonzero and the return value of File is undefined. It accepts flags.*/
File open_file(const char *path, int mode, int *err);
int close_file(File file);

int lock_file(File f, fsize_t off, fsize_t len);
int unlock_file(File f, fsize_t off, fsize_t len);

/*Returns the size of the file at path. the value returned by this funtion in fsize
is guaranteed to be >= 0. The actual type of fsize_t is implementation defined.
@return 0 on success and fsize is set to the file size. non 0 error code on failure*/
int get_file_size(const char *path, fsize_t *fsize);
int fget_file_size(File f, fsize_t *fsize);

/*Deletes the file at path. Returns 0 on success, nonzero on failure*/
int delete_file(const char *path);
/*Changes the working directory of the process. Returns 0 on success, non 0 on failure*/
int change_dir(const char *path);
/*Returns the pwd absolute path. The buffer is malloc'd so the caller
 *should call free on the buffer once he's done using it.*/
char* get_cwd();
/*Returns the file's absolute path. The buffer is malloc'd so the caller
 *should call free on the buffer once he's done using it.*/
char* get_abs(const char *path);
/*Renames the file old_name to new_name*/
int rename_file(const char *old_name, const char *new_name);

#endif
