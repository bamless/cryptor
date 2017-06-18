#include "socket.h"
#include "threadpool.h"
#include "logging.h"
#include "utilsCompat.h"
#include "error.h"
#include "files.h"
#include "stringbuf.h"

#include <stdlib.h>
#include <inttypes.h>

int main() {
	StringBuffer *sb = sbuf_create();
	sbuf_appendstr(sb, "HelloHello");
	sbuf_appendstr(sb, "HelloHello");
	sbuf_appendstr(sb, "HelloHello");
	sbuf_appendstr(sb, "HelloHello");
	sbuf_appendstr(sb, "HelloFINDME");
	sbuf_appendstr(sb, "HelloHello");
	sbuf_appendstr(sb, "HelloHello");
	printf("%s\n", sbuf_get_backing_buf(sb));
	printf("%s\n", sbuf_strstr(sb, "FIND"));
	sbuf_clear(sb);
	sbuf_appendstr(sb, "Hey");
	printf("%s\n", sbuf_get_backing_buf(sb));
	sbuf_destroy(sb);
}
