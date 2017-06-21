#include "socket.h"
#include "threadpool.h"
#include "logging.h"
#include "utilsCompat.h"
#include "error.h"
#include "files.h"
#include "stringbuf.h"

#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include <windows.h>
// #include <sys/mman.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>

int main() {
	fsize_t s;
	if(get_file_size("D:/Fabrizio/Pictures/crackstation.txt", &s)) {
		perr("error get_file_size");
		exit(1);
	}
	printf("%"PRIu64"\n", (intmax_t) s);

	s = ceil(s/4.) * 4;

	printf("%"PRIu64"\n", (intmax_t) s);

	HANDLE hFile = CreateFile("D:/Fabrizio/Pictures/crackstation.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE mapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, (DWORD) (s >> 32), (DWORD) s, NULL);
	if(mapping == NULL) {
		perr("Error CreateFileMapping");
		exit(1);
	}
	int *map = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
	if(map == NULL) {
		perr("Error MapViewOfFile");
		exit(1);
	}

	printf("%"PRIu64"\n", (intmax_t) s);
	int key = 25678;

	fsize_t i;
	for(i = 0; i < ceil(s/4.); i++) {
	 	map[i] ^= key;
	}
	printf("%"PRIu64"\n", (intmax_t) i);

	FlushViewOfFile(map, 0);
	UnmapViewOfFile(map);
	CloseHandle(hFile);
	CloseHandle(mapping);

	//int fd = open("/home/fabrizio/prova.txt", O_RDWR);
	// int fd = open("/mnt/HDD/Fabrizio/Pictures/crackstation6GB.txt", O_RDWR);
	// if(fd < 0) logs("error open");
	// int key = 25678;
	// int *map = (int *) mmap(NULL, ceil(s/4.) * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	// if(!map) logs("error mmap");
	// for(fsize_t i = 0; i < ceil(s/4.); i++) {
	// 	map[i] ^= key;
	// }
	// munmap(map, s);

}
