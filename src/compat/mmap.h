#ifndef MMAP_H
#define MMAP_H

#include "files.h"

typedef struct MemoryMap MemoryMap;

/*Maps a file of length 'length' in memory*/
MemoryMap *memory_map(File f, fsize_t length);
/*Unmaps a previously mmapped file*/
int memory_unmap(MemoryMap *mmap);
/*
 * Maps a view of a file mapping into the address space of a calling process of
 * length 'len' starting at position 'off'. The 'off' argument must be a multiple
 * of the page size.
 * @return the address of the mapped view
 */
void *mmap_mapview(MemoryMap *mmap, fsize_t off, fsize_t len);
/*
 * Unmaps a view of a file.
 * @return 0 on success, non-zero on failure
 */
int mmap_unmapview(void *view);

#endif
