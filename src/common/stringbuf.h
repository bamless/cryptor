#ifndef STRINGBUF_H
#define STRINGBUF_H

#include <stdlib.h>

/*
 * Auto-resizing char buffer.
 */
typedef struct StringBuffer StringBuffer; //opaque type, to provide encapsulation

StringBuffer* sbuf_create();
void sbuf_destroy(StringBuffer *sbuf);

void sbuf_clear(StringBuffer *sbuf);
char* sbuf_get_backing_buf(StringBuffer *sbuf);
char* sbuf_strstr(StringBuffer *sbuf, const char *needle);
size_t sbuf_get_len(StringBuffer *sbuf);
char* sbuf_detach(StringBuffer *sbuf);
void sbuf_printf(StringBuffer *sbuf, const char *fmt, ...);
int sbuf_endswith(StringBuffer *sbuf, const char *str);
void sbuf_append(StringBuffer *sbuf, const char *str, size_t len);
void sbuf_appendstr(StringBuffer *sbuf, const char *str);
void sbuf_truncate(StringBuffer *sbuf, size_t len);

#endif
