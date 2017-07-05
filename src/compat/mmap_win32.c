#include "mmap.h"
#include "files.h"

#include <stdlib.h>
#include <windows.h>

struct MemoryMap {
    HANDLE mapping;
    DWORD prot;
};

MemoryMap *memory_map(File f, fsize_t length, int flags) {
    DWORD prot = 0;
    if(flags & MMAP_WRITE) {
        prot = PAGE_READWRITE;
    } else if(flags & MMAP_READ) {
        prot = PAGE_READONLY;
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

    return mmap;
}

int memory_unmap(MemoryMap *mmap) {
    int err = CloseHandle(mmap->mapping) ? 0 : 1;
    free(mmap);
    return err;
}

void *mmap_mapview(MemoryMap *mmap, fsize_t off, fsize_t length) {
    DWORD acc = 0;
    if(mmap->prot == PAGE_READWRITE)
        acc = FILE_MAP_WRITE;
    else
        acc = FILE_MAP_READ;
    return MapViewOfFile(mmap->mapping, acc, (DWORD) (off >> 32), (DWORD) off, length);
}

int mmap_unmapview(void *view) {
    return UnmapViewOfFile(view) ? 0 : 1;
}
