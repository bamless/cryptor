#ifndef MMAP_H
#define MMAP_H

#include "files.h"

typedef struct MemoryMap MemoryMap;

MemoryMap *memory_map(File f, fsize_t off, fsize_t length);
int memory_unmap(MemoryMap *mmap);
void *mmap_getaddr(MemoryMap *mmap);


#endif
