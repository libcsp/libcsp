#include <stdlib.h>

#include <csp/csp.h>
#include <csp/csp_malloc.h>

void * csp_malloc(size_t size) {
    return malloc(size);
}

void csp_free(void *ptr) {
    free(ptr);
}

