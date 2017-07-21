#include "encrypt.h"
#include "files.h"
#include "mmap.h"
#include "error.h"

#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <omp.h>

#define INT_LEN sizeof(int)   //int length in bytes
#define PAR_BLCK (256 * 1024) //dimension, in bytes, of a thread unit of execution

#ifdef _WIN32
static int rand_r(unsigned int *seed);
#endif

static MemoryMap* generate_key(fsize_t size, unsigned int seed, File backing);
static int create_cipher_file(const char *name, fsize_t size, File *out);

int encrypt(File plainfd, const char *out_name, unsigned int key_seed) {
    if(out_name == NULL) return -1;

    fsize_t size;
    if(fget_file_size(plainfd, &size)) {
        perr("Error encrypt");
        return -1;
    }

    //create a temporary file for the one-time pad key and maps it
    File backing = create_tmp_file();
    MemoryMap *key_map = generate_key(size, key_seed, backing); //generate the key
    if(!key_map) {
        close_file(backing);
        return -1;
    }
    //map the plaintext and ciphertext file
    MemoryMap *plain = memory_map(plainfd, size, MMAP_READ, MMAP_PRIVATE);
    if(!plain) {
        perr("Error mapping the file for encrypting");
        memory_unmap(key_map);
        close_file(backing);
        return -1;
    }
    //create and mmap the ciphertext file
    File cipherfd;
    if(create_cipher_file(out_name, size, &cipherfd)) {
        perr("Error mapping the cipher file");
        memory_unmap(plain);
        memory_unmap(key_map);
        close_file(backing);
    }
    MemoryMap *cipher = memory_map(cipherfd, size, MMAP_READ | MMAP_WRITE, MMAP_SHARED);
    if(!cipher) {
        perr("Error mapping the cipher file");
        memory_unmap(plain);
        memory_unmap(key_map);
        close_file(backing);
        close_file(cipherfd);
        delete_file(out_name); //delete the cipher file on error
        return -1;
    }

    //the number of 256Kb chunks in the file
    fsize_t num_chunks = ceil(size/((float) PAR_BLCK));
    //finally crypt the file
    int result = 0;
    #pragma omp parallel for
    for(fsize_t n = 0; n < num_chunks; n++) {
        fsize_t from = n * PAR_BLCK;
        fsize_t len = (from + PAR_BLCK) > size ? size - from : PAR_BLCK;

        //map views of the size of the chunk
        int *plain_chunk = mmap_mapview(plain, from, len);
        int *cipher_chunk = mmap_mapview(cipher, from, len);
        int *key = mmap_mapview(key_map, from, len);
        if(!cipher_chunk || !plain_chunk || !key) {
            perr("Error getting mapped file view");
            #pragma omp atomic write
            result = -1; //error
        }

        //crypt the bytes of the chunk in 4 bytes groups
        fsize_t len32 = floor(len/(float) INT_LEN);
        for(int i = 0; i < len32; i++) {
            cipher_chunk[i] = plain_chunk[i] ^ key[i];
        }

        //if the file is not a multiple of 4 then encrypt the last bytes 1 byte at a time
        int remainder;
        if((remainder = len % INT_LEN) != 0) {
            char *key_bytes = (char *) key;
            char *plain_bytes = (char *) plain_chunk;
            char *cipher_bytes = (char *) cipher_chunk;
            for(int i = len - remainder; i <= len; i++) {
                cipher_bytes[i] = plain_bytes[i] ^ key_bytes[i];
            }
        }

        mmap_unmapview(plain_chunk);
        mmap_unmapview(cipher_chunk);
        mmap_unmapview(key);
    }

    memory_unmap(plain);
    memory_unmap(cipher);
    memory_unmap(key_map);

    unlock_file(cipherfd, 0, size);
    close_file(cipherfd);
    close_file(backing);
    if(result == -1) delete_file(out_name);
    return result;
}

/*
 * Generates a one-time pad key and returns a memory map to the location containing it.
 * the size of the region of memory is the closest multiple of 4 to size rounding up.
 */
static MemoryMap* generate_key(fsize_t size, unsigned int seed, File backing) {
    size = ceil(size/(float) INT_LEN) * INT_LEN;
    MemoryMap *key_mmap = memory_map(backing, size, MMAP_READ | MMAP_WRITE, MMAP_SHARED);
    if(!key_mmap) return NULL;

    int *key = mmap_mapview(key_mmap, 0, size);
    if(!key) return NULL;
    for(fsize_t i = 0; i < size / INT_LEN; i++) {
        key[i] = rand_r(&seed);
    }
    mmap_unmapview(key);

    return key_mmap;
}

/*
 * Creates the cipher file and locks it over size 'size'. The size should be the same as the plaintext file
 * @return 0 on success, non 0 on failure.
 */
static int create_cipher_file(const char *name, fsize_t size, File *out) {
    int err = 0;
    *out = open_file(name, READ | WRITE | CREATE, &err);
    if(err) {
        perr("Error creating the ciphertext file");
        return -1;
    }
    if(lock_file(*out, 0, size)) {
        perr("Error locking the ciphertext file");
        close_file(*out);
        return -1;
    }
    return 0;
}

//Mingw-w64 does not seem to have rand_r implemented. The following implementation is
//taken from the Mingw source code on sourceforge
#ifdef _WIN32
/**Thread safe random number generator.*/
static int rand_r(unsigned int *seed) {
        long k;
        long s = (long)(*seed);
        if (s == 0)
            s = 0x12345987;
        k = s / 127773;
        s = 16807 * (s - k * 127773) - 2836 * k;
        if (s < 0)
            s += 2147483647;
        (*seed) = (unsigned int)s;
        return (int)(s & RAND_MAX);
}
#endif
