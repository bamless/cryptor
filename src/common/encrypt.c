#include "encrypt.h"
#include "files.h"
#include "mmap.h"
#include "error.h"

#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <omp.h>

#define INT_LEN sizeof(int) //int length in bytes

//dimension, in bytes, of a thread unit of execution
#define PAR_BLCK (256 * 1024)

int encrypt(File plainfd, const char *out_name, int *key, int key_len) {
    //if key_len not a power of 2 or greater than PAR_BLCK or out_name is NULL
    if(((key_len * INT_LEN) & (key_len * INT_LEN - 1)) != 0 ||
                        (key_len * INT_LEN) > PAR_BLCK || out_name == NULL)
        return -1;

    fsize_t size;
    if(fget_file_size(plainfd, &size)) return -1;

    int err = 0;
    File cipherfd = open_file(out_name, READ | WRITE | CREATE, &err);
    if(err) return -1;
    if(lock_file(cipherfd, 0, size)) {
        close_file(cipherfd);
        return -1;
    }

    MemoryMap *plain = memory_map(plainfd, size, MMAP_READ);
    MemoryMap *cipher = memory_map(cipherfd, size, MMAP_READ | MMAP_WRITE);
    if(!cipher || !plain) {
        unlock_file(cipherfd, 0, size);
        close_file(cipherfd);
        return -1;
    }

    fsize_t num_chunks = ceil(size/((float) PAR_BLCK));
    #pragma omp parallel for
    for(fsize_t n = 0; n < num_chunks; n++) {
        fsize_t from = n * PAR_BLCK;
        fsize_t len = (from + PAR_BLCK) > size ? size - from : PAR_BLCK;

        int *plain_chunk = mmap_mapview(plain, from, len);
        int *cipher_chunk = mmap_mapview(cipher, from, len);
        if(!cipher_chunk || !plain_chunk) perr("Error encrypt");

        fsize_t len32 = ceil(len/(float) INT_LEN);
        for(int i = 0; i < len32; i++) {
            cipher_chunk[i] = plain_chunk[i] ^ key[i % key_len];
        }

        mmap_unmapview(plain_chunk);
        mmap_unmapview(cipher_chunk);
    }

    memory_unmap(plain);
    memory_unmap(cipher);
    unlock_file(cipherfd, 0, size);
    close_file(cipherfd);
    return 0;
}
