#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <inttypes.h>
#include <limits.h>

/*
 * This header contains various constants used in the porocol implementation.
 */

#define LSTF "LSTF"
#define LSTR "LSTR"
#define ENCR "ENCR"
#define DECR "DECR"

#define CRYPTOR_MAX_CMD_STRLEN 4

#define RETOK_INT 200
#define RETMORE_INT 300
#define RETERR_INT 400
#define RETERRTRANS_INT 500
#define RETOK "200"
#define RETMORE "300"
#define RETERR "400"
#define RETERRTRANS "500"

#define CRYPTOR_MAX_B10_RETCODE_STRLEN 3

#define CRYPTOR_DEFAULT_PORT 8888 //the server default port

/**
 * Calculate the approximate maximum base 10 length of the given integer type.
 * This macro evaluates to a constant, as to permit static allocation of buffer
 */
#define __MAX_STRLEN_FOR_UNSIGNED_TYPE(t) \
    (((((sizeof(t) * CHAR_BIT)) * 1233) >> 12) + 1)

#define __MAX_STRLEN_FOR_SIGNED_TYPE(t) \
    (__MAX_STRLEN_FOR_UNSIGNED_TYPE(t) + 1)

#define MAX_STRLEN_FOR_INT_TYPE(t) \
    (((t) -1 < 0) ? __MAX_STRLEN_FOR_SIGNED_TYPE(t) \
                  : __MAX_STRLEN_FOR_UNSIGNED_TYPE(t))

#endif
