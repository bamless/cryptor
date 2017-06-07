#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "files.h"
#include "logging.h"
#include "fileUtils.h"

int main() {
	// Dir *d = open_dir("C:\\Games");
	// if(!d) {
	// 	perr("error");
	// 	exit(-1);
	// }
	// close_dir(d);
	recursive_list("D:/Fabrizio/Pictures");
}
