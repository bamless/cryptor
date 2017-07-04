#ifndef ENCRYPT_H
#define ENCRYPT_H

#include "files.h"
/*
 * Encrypt a file f with a key using a simple XOR encryption scheme.
 * @arg f the file to be encrypted, it must be opened with read/write permissions
 * @arg key a pointer to the key to be used for the encryption
 * @arg key_len the length of the 'key' argument
 * @return 0 on success, non-zero on failure
 */
int encrypt(File plainfd, const char *out_name, int *key, int key_len);

#endif
