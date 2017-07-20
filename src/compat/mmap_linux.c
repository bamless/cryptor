#include "mmap.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

struct MemoryMap {
    void *map;
    size_t length;
};

MemoryMap *memory_map(File f, fsize_t length, int flags) {
    if(length < 0) return NULL;
    fsize_t size;
    if(fget_file_size(f, &size)) {
        return NULL;
    }
    if(length > size) {
        //stretch the file to the length of the file mapping
        if(lseek(f, length-1, SEEK_SET) == -1) return NULL;
        if(write(f, "", 1) == -1) return NULL;
    }

    int mmap_flags = 0;
    if(flags & MMAP_READ) {
        mmap_flags |= PROT_READ;
    }
    if(flags & MMAP_WRITE) {
        mmap_flags |= PROT_WRITE;
    }
    if(!flags) {
        mmap_flags |= PROT_NONE;
    }

    void *map = mmap(NULL, length, mmap_flags, MAP_SHARED, f, 0);
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
