#include "mmap.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

struct MemoryMap {
    void *map;
    size_t length;
};

MemoryMap *memory_map(File f, fsize_t length, int flags, enum MapMode mode) {
    if(length < 0) return NULL;
    fsize_t size;
    if(fget_file_size(f, &size)) {
        return NULL;
    }
    if(length > size && !(flags & MMAP_ANONYMOUS)) {
        //stretch the file to the length of the file mapping
        if(ftruncate(f, length)) return NULL;
    }

    int mmap_prot = 0;
    if(flags & MMAP_READ) {
        mmap_prot |= PROT_READ;
    }
    if(flags & MMAP_WRITE) {
        mmap_prot |= PROT_WRITE;
    }
    if(!(flags & MMAP_READ) && !(flags & MMAP_WRITE)) {
        mmap_prot |= PROT_NONE;
    }

    int mmap_flags = 0;
    if(flags & MMAP_ANONYMOUS) {
        mmap_flags |= MAP_ANONYMOUS;
        f = -1; //according to man, some implementation may require a -1 fd on MAP_ANONYMOUS
    }
    if(mode == MMAP_PRIVATE) {
        mmap_flags |= MAP_PRIVATE;
    } else if(mode == MMAP_SHARED) {
        mmap_flags |= MAP_SHARED;
    }

    void *map = mmap(NULL, length, mmap_prot, mmap_flags, f, 0);
    if(map == MAP_FAILED) return NULL;

    MemoryMap *mmap = malloc(sizeof(MemoryMap));
    if(!mmap) {
        munmap(map, length);
        return NULL;
    }
    mmap->length = length;
    mmap->map = map;

    return mmap;
}

int memory_unmap(MemoryMap *mmap) {
    int err = munmap(mmap->map, mmap->length);
    free(mmap);
    return err;
}

void *mmap_mapview(MemoryMap *mmap, fsize_t off, fsize_t len) {
    //linux doesn't have file views of mapped files, so return the
    //mapping base address plus the offset requested.
    return mmap->map + off;
}

int mmap_unmapview(void *view) {
    //we don't need to unmap a view on unix
    return 0;
}
