#include "files.h"
#include "error.h"

int main() {
	if(delete_file("prova.txt"))
		perr_exit("Error");
}
