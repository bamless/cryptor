#include "encrypt.h"
#include "files.h"
#include "mmap.h"
#include "error.h"

#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <omp.h>

//dimension, in bytes, of a thread unit of execution
#define PAR_BLCK (256 * 1024)

int encrypt(File file, int *key, int key_len) {
    if(((key_len * 4) & (key_len * 4 - 1)) != 0 || (key_len * 4) > PAR_BLCK)
        return -1;

    fsize_t size;
    if(fget_file_size(file, &size)) {
        return -1;
    }
    size = ceil(size/4.) * 4;

    MemoryMap *mmap = memory_map(file, size);
    if(!mmap) return -1;

    fsize_t num_chunks = ceil(size/((float) PAR_BLCK));

    //int numthreads = num_chunks > 4 ? 4 : num_chunks;
    #pragma omp parallel for //num_threads(numthreads)
    for(fsize_t n = 0; n < num_chunks; n++) {
        fsize_t from = n * PAR_BLCK;
        fsize_t len = (from + PAR_BLCK) > size ? size - from : PAR_BLCK;

        int *chunk = mmap_mapview(mmap, from, len);
        if(chunk == NULL) perr("Error encrypt");

        fsize_t len32 = ceil(len/4.);
        for(int i = 0; i < len32; i++) {
            chunk[i] ^= key[i % key_len];
        }

        mmap_unmapview(chunk);
    }

    memory_unmap(mmap);
    return 0;
}
