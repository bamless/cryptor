#include "mmap.h"

#include <stdlib.h>
#include <sys/mman.h>

struct MemoryMap {
    void *map;
    size_t length;
};

MemoryMap *memory_map(File f, fsize_t off, fsize_t length) {
    if(length < 0) return NULL;
    void *map = mmap(NULL, (size_t) length, PROT_READ | PROT_WRITE, MAP_SHARED, f, off);
    if(map == MAP_FAILED) return NULL;

    MemoryMap *mmap = malloc(sizeof(MemoryMap));
    if(!mmap) return NULL;
    mmap->length = (size_t) length;
    mmap->map = map;

    return mmap;
}

int memory_unmap(MemoryMap *mmap) {
    int err = munmap(mmap->map, mmap->length);
    free(mmap);
    return err;
}

void *mmap_getaddr(MemoryMap *mmap) {
    return mmap->map;
}
