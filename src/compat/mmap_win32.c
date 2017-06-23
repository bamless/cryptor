#include "mmap.h"

#include <stdlib.h>
#include <windows.h>

struct MemoryMap {
    HANDLE mapping;
    void *map;
};

MemoryMap *memory_map(File f, fsize_t off, fsize_t length) {
    HANDLE mapping = CreateFileMapping(f, NULL, PAGE_READWRITE, (DWORD) (length >> 32), (DWORD) length, NULL);
    if(mapping == NULL) return NULL;
    void *map = MapViewOfFile(mapping, FILE_MAP_WRITE, (DWORD) (off >> 32), (DWORD) off, length);
    if(map == NULL) return NULL;

    MemoryMap *mmap = malloc(sizeof(MemoryMap));
    if(!mmap) return NULL;
    mmap->mapping = mapping;
    mmap->map = map;

    return mmap;
}

int memory_unmap(MemoryMap *mmap) {
    int err = FlushViewOfFile(mmap->map, 0) ? 0 : 1;
    err |= UnmapViewOfFile(mmap->map) ? 0 : 1;
    err |= CloseHandle(mmap->mapping) ? 0 : 1;
    free(mmap);
    return err;
}

void *mmap_getaddr(MemoryMap *mmap) {
    return mmap->map;
}
