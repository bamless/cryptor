#include "socket.h"
#include "threadpool.h"
#include "logging.h"
#include "utilsCompat.h"
#include "error.h"
#include "files.h"
#include "stringbuf.h"
#include "fileUtils.h"

#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main() {
	// fsize_t s;
	// get_file_size("/mnt/HDD/Fabrizio/Pictures/pswd.txt", &s);
	// printf("%"PRIu64"\n", (intmax_t) s);
	// printf("%f\n", ceil(s/4.) * 4);
	// //int fd = open("/home/fabrizio/prova.txt", O_RDWR);
	// int fd = open("/mnt/HDD/Fabrizio/Pictures/pswd.txt", O_RDWR);
	// if(fd < 0) logs("error open");
	// int key = 25678;
	// int *map = (int *) mmap(NULL, ceil(s/4.) * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	// if(!map) logs("error mmap");
	// for(int i = 0; i < ceil(s/4.); i++) {
	// 	map[i] ^= key;
	// }
	StringBuffer *sb = sbuf_create();
	sbuf_appendstr(sb, "CIAO");
	printf("%s\n", sbuf_get_backing_buf(sb));
	sbuf_printf(sb, "%s %d %s", "hey", 10, "no");
	printf("%s\n", sbuf_get_backing_buf(sb));
}
