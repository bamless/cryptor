#include "encrypt.h"
#include "files.h"
#include "mmap.h"
#include "error.h"

#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <omp.h>

#include <stdio.h>

#define INT_LEN sizeof(int)   //int length in bytes
#define PAR_BLCK (256 * 1024) //dimension, in bytes, of a thread unit of execution

#ifdef _WIN32
static int rand_r(unsigned int *seed);
#endif

static int create_cipher_file(const char *name, fsize_t size, File *out);

int encrypt(File plainfd, const char *out_name, unsigned int key_seed) {
	if(out_name == NULL) return -1;

	fsize_t size;
	if(fget_file_size(plainfd, &size)) {
		perr("Error getting plaintext file size");
		return -1;
	}

	//map the plaintext and ciphertext file
	MemoryMap *plain = memory_map(plainfd, size, MMAP_READ, MMAP_PRIVATE);
	if(!plain) {
		perr("Error mapping the file for encryption");
		return -1;
	}

	//create and mmap the ciphertext file
	File cipherfd;
	if(create_cipher_file(out_name, size, &cipherfd)) {
		perr("Error mapping the cipher file");
		memory_unmap(plain);
		return -1;
	}
	MemoryMap *cipher = memory_map(cipherfd, size, MMAP_READ | MMAP_WRITE, MMAP_SHARED);
	if(!cipher) {
		perr("Error mapping the cipher file");
		memory_unmap(plain);
		close_file(cipherfd);
		delete_file(out_name); //delete the cipher file on error
		return -1;
	}

	//the number of 256Kb chunks in the file
	size_t num_chunks = ceil(size/(float) PAR_BLCK);

	//generates a new seed for every thread (1 per chunk) using the supplied seed
	unsigned int *seeds = malloc(sizeof(unsigned int) * num_chunks);
	for(size_t i = 0; i < num_chunks; i++) {
		seeds[i] = rand_r(&key_seed);
	}
	
	//finally encrypt the file
	int result = 0;
	#pragma omp parallel for
	for(size_t n = 0; n < num_chunks; n++) {
		fsize_t from = n * PAR_BLCK;
		fsize_t len = (from + PAR_BLCK) > size ? size - from : PAR_BLCK;

		//map views of the size of the chunk
		int *plain_chunk = mmap_mapview(plain, from, len);
		int *cipher_chunk = mmap_mapview(cipher, from, len);

		if(!cipher_chunk || !plain_chunk) {
			perr("Error getting mapped file view");
			#pragma omp atomic write
			result = -1; //error
		}

		//encrypt the bytes of the chunk in 4 bytes groups
		int len_int = floor(len/(float) INT_LEN);
		for(int i = 0; i < len_int; i++) {
			cipher_chunk[i] = plain_chunk[i] ^ rand_r(&seeds[n]);
		}

		//if the file is not a multiple of 4 then encrypt the last bytes 1 at a time
		int remainder;
		if((remainder = len % INT_LEN) != 0) {
			int k = rand_r(&seeds[n]);
			for(int i = len - remainder; i < len; i++) {
				((char *) cipher_chunk)[i] = ((char *) plain_chunk)[i] ^ ((char *) &k)[remainder - (len - i)];
			}
		}

		mmap_unmapview(plain_chunk);
		mmap_unmapview(cipher_chunk);
	}

	memory_unmap(plain);
	memory_unmap(cipher);
	free(seeds);

	unlock_file(cipherfd, 0, size);
	close_file(cipherfd);

	if(result == -1)
		delete_file(out_name);

	return result;
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
