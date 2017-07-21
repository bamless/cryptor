#ifndef ENCRYPT_H
#define ENCRYPT_H

#include "files.h"
/*
 * Encrypts a file f with a one-time pad key generated randomily from seed.
 * @arg plainfd the file to be encrypted.
 * @arg out_name the name of the file that will contain the cipertext of `plainfd`
 * @arg key_seed the seed used to generate a one-time pad key for the file plainfd
 * @return 0 on success, non-zero on failure
 */
int encrypt(File plainfd, const char *out_name, unsigned int key_seed);

#endif
