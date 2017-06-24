#include "encrypt.h"
#include "files.h"
#include "mmap.h"
#include "logging.h"

#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <omp.h>

//dimension, in bytes, of a thread unit of execution
#define PAR_BLCK (256 * 1024)

int encrypt(File file, int *key, int key_len) {
    if( ((key_len * 4) & (key_len * 4 - 1)) != 0 || (key_len * 4) > PAR_BLCK)
        return -1;

    fsize_t size;
    if(fget_file_size(file, &size)) {
        return -1;
    }
    size = ceil(size/4.) * 4;

    MemoryMap *mmap = memory_map(file, 0, size);
    if(!mmap) return -1;

    int *map = mmap_getaddr(mmap);

    fsize_t iter = ceil(size/4.);
    #pragma omp parallel for schedule(static, (int) ceil(PAR_BLCK/4.))
    for(fsize_t i = 0; i < iter; i++) {
        map[i] ^= key[i % key_len];
    }

    if(memory_unmap(mmap)) {
        return -1;
    }
    return 0;
}
