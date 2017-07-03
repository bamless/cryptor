#include "socket.h"
#include "threadpool.h"
#include "logging.h"
#include "utilsCompat.h"
#include "thread.h"
#include "error.h"
#include "files.h"
#include "stringbuf.h"
#include "mmap.h"
#include "encrypt.h"

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

static void thread(void *args) {
	int err;
	File file = open_file("/mnt/HDD/Fabrizio/Pictures/crackstation3GB.txt", READ | WRITE, &err);
	if(err) {
		perr("Error");
		exit(1);
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

	int key[8] = {25678, 45656, 34566, 9876532, 3254543, 7862331, 4312, 83,}; //key for testing

	if(encrypt(file, key, 8)) {
		elog("error while encrypting the file");
	}

	unlock_file(file, 0, s);
	close_file(file);
}

int main() {
	Thread t;
	thread_create(&t, &thread, NULL);
	thread_join(&t);
}
