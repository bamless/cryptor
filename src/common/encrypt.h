#ifndef ENCRYPT_H
#define ENCRYPT_H

#include "files.h"
/*
 * Encrypts a file f with a key using a simple XOR encryption scheme.
 * @arg plainfd the file to be encrypted.
 * @arg out_name the name of the file that will contain the cipertext of `plainfd`
 * @arg key a pointer to the key to be used for the encryption
 * @arg key_len the length of the 'key' argument. Must be a power of 2
 * @return 0 on success, non-zero on failure
 */
int encrypt(File plainfd, const char *out_name, int *key, int key_len);

#endif
