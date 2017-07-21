#include "mmap.h"
#include "files.h"

#include <stdlib.h>
#include <windows.h>

struct MemoryMap {
    HANDLE mapping;
    DWORD prot;
    enum MapMode mode;
};

MemoryMap *memory_map(File f, fsize_t length, int flags, enum MapMode mode) {
    DWORD prot = 0;
    if(flags & MMAP_WRITE) {
        prot = mode == MMAP_PRIVATE ? PAGE_WRITECOPY : PAGE_READWRITE;
    } else if(flags & MMAP_READ) {
        prot = PAGE_READONLY;
    }
    if(flags & MMAP_ANONYMOUS) {
        f = INVALID_HANDLE_VALUE; //no backing file
    }
    HANDLE mapping = CreateFileMapping(f, NULL, prot, (DWORD) (length >> 32), (DWORD) length, NULL);
    if(mapping == NULL) return NULL;

    MemoryMap *mmap = malloc(sizeof(MemoryMap));
    if(!mmap) {
        CloseHandle(mapping);
        return NULL;
    }
    mmap->mapping = mapping;
    mmap->prot = prot;
    mmap->mode = mode;

    return mmap;
}

int memory_unmap(MemoryMap *mmap) {
    int err = CloseHandle(mmap->mapping) ? 0 : 1;
    free(mmap);
    return err;
}

void *mmap_mapview(MemoryMap *mmap, fsize_t off, fsize_t length) {
    DWORD acc = 0;
    if(mmap->prot == PAGE_READWRITE) {
        acc = mmap->mode == MMAP_PRIVATE ? FILE_MAP_COPY : FILE_MAP_WRITE;
    } else {
        acc = FILE_MAP_READ;
    }
    return MapViewOfFile(mmap->mapping, acc, (DWORD) (off >> 32), (DWORD) off, length);
}

int mmap_unmapview(void *view) {
    return UnmapViewOfFile(view) ? 0 : 1;
}
