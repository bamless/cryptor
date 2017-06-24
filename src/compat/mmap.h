#ifndef MMAP_H
#define MMAP_H

#include "files.h"

typedef struct MemoryMap MemoryMap;

MemoryMap *memory_map(File f, fsize_t length);
int memory_unmap(MemoryMap *mmap);
void *mmap_mapview(MemoryMap *mmap, fsize_t off, fsize_t len);
int mmap_unmapview(void *view);

#endif
