#include "socket.h"
#include "threadpool.h"
#include "logging.h"
#include "utilsCompat.h"
#include "error.h"
#include "files.h"
#include "stringbuf.h"
#include "fileUtils.h"

#include <stdlib.h>
#include <inttypes.h>

int main() {
	fsize_t fsize;
	if(get_file_size("D:\\Fabrizio\\Musica/prova/asdf.txt", &fsize)) {
		logs("coddio");
	}
}
