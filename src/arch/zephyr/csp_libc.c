

#include <stdlib.h>

int sscanf(const char * s, const char * format, ...) {
	return 0;
}

int rand_r(unsigned int * seedp) {
	srand(*seedp);
	return rand();
}
