#include "socket.h"
#include "threadpool.h"
#include "logging.h"
#include "utilsCompat.h"
#include "error.h"
#include "files.h"
#include "stringbuf.h"
#include "mmap.h"

#include <omp.h>
#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
// #include <windows.h>

// #include <sys/mman.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>

int main() {

	int err;
	File file = open_file("/home/fabrizio/tux.bmp", READ | WRITE, &err);
	if(err) {
		perr("Error");
		return 1;
	}

	fsize_t s;
	if(fget_file_size(file, &s)) {
		perr("error get_file_size");
		exit(1);
	}

	printf("%"PRIu64"\n", (intmax_t) s);

	s = ceil(s/4.) * 4;

	printf("%"PRIu64"\n", (intmax_t) s);

	lock_file(file, 0, s);

	//Encrypt

	int key[8] = {214748347, 5465489, 27933456, 940211, 1274325, 786888, 5468392, 7876626}; //key for testing

	MemoryMap *mmap = memory_map(file, 0, s);
	if(!mmap) perr("Error");

	int *map = mmap_getaddr(mmap);

	int num_threads = ceil(s/1024.);
	omp_set_num_threads(num_threads);

	#pragma omp parallel for
	for(int n = 0; n < num_threads; n++) {
		int from = n * 1024;
		int len = (from + 1024) > s ? s - from : 1024;
		len = ceil(len/4.);

		printf("from byte %d len (32bit) %d\n", from, len);

		if(omp_in_parallel()) {
			printf("Thread %d\n", n);
		}

		int *chunk = &map[(int) ceil(from/4.)];
		for(int i = 0; i < len; i++) {
			chunk[i] ^= key[i % 8];
		}
	}

	memory_unmap(mmap);

	unlock_file(file, 0, s);
	close_file(file);

	return 0;


}
