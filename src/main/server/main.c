#include <stdlib.h>
#include "files.h"
#include "error.h"
#include "logging.h"

int main() {
	if(!open_dir("inexistent")) {
		perr_exit("errore");
	}
}
