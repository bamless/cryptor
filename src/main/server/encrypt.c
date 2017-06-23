#include "encrypt.h"
#include "files.h"
#include "mmap.h"
#include "logging.h"

#include <inttypes.h>

//dimension, in bytes, of a thread unit of execution
#define PAR_BLCK 1024

int encrypt(File file, int *key, int key_len) {

    if( ((key_len * 4) & (key_len * 4 - 1)) != 0 || (key_len * 4) > PAR_BLCK)
        return -1;

    fsize_t size;
    if(fget_file_size(file, &size)) {
        return -1;
    }

    MemoryMap *mmap = memory_map(file, 0, size);
    if(!mmap) return -1;

    int *map = mmap_getaddr(mmap);

    fsize_t num_threads = ceil(size/1024.);
    if(num_threads > INT_MAX) return - 1;   //file is too big

    omp_set_num_threads(num_threads);

    #pragma omp parallel for
    for(int n = 0; n < num_threads; n++) {
        int from = n * 1024;
        int len = (from + 1024) > s ? s - from : 1024;
        len = ceil(len/4.);

        printf("from byte %d len (32bit) %d\n", from, len);

        if(omp_in_parallel()) {
            printf("Thread %d\n", n);
        }

        int *chunk = &map[(int) ceil(from/4.)];
        for(int i = 0; i < len; i++) {
            chunk[i] ^= key[i % 8];
        }
    }
}
