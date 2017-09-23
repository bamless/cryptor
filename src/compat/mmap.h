#ifndef MMAP_H
#define MMAP_H

#include "files.h"

/*
 * Cross platform memory map interface.
 */

typedef struct MemoryMap MemoryMap;

/*The memory-mapped pages can be read*/
#define MMAP_READ 1
/*The memory-mapped pages can be executed*/
#define MMAP_WRITE 2
/*The memory map is not backed by any file, the File argument is ignored*/
#define MMAP_ANONYMOUS 4

enum MapMode {
	MMAP_SHARED,
	MMAP_PRIVATE
};

/*
 * Maps a file of length 'length' in memory.
 * @arg f the file to be memory mapped
 * @arg length the length of f
 * @arg mode MMAP_READ for readonly, MMAP_WRITE for wronly, or bitwise or of the two
 * @return a MemoryMap struct on success, NULL on failure
 */
MemoryMap *memory_map(File f, fsize_t length, int flags, enum MapMode mode);
/*
 * Unmaps a previously mmapped file.
 * @return 0 on succsess, non 0 otherwise
 */
int memory_unmap(MemoryMap *mmap);
/*
 * Maps a view of a file mapping into the address space of a calling process of
 * length 'len' starting at position 'off'. The 'off' argument must be a multiple
 * of the page size. This function is here mainly for compatibility reasons. On
 * windows this allows to have only a certain amount of the file in the process'
 * address space (for working in blocks).
 * On linux all this function does is adding the offset to the base address
 * returned by the mmap function.
 * @return the address of the mapped view, NULL on failure
 */
void *mmap_mapview(MemoryMap *mmap, fsize_t off, fsize_t len);
/*
 * Unmaps a view of a file.
 * @return 0 on success, non-zero on failure
 */
int mmap_unmapview(void *view);

#endif
