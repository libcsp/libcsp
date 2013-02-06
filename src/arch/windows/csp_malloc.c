#include <stdlib.h>

void * csp_malloc(size_t size) {
	return malloc(size);
}

void csp_free(void * ptr) {
	free(ptr);
}
