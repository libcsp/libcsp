#include <stdint.h>

/* FreeRTOS includes */
#include <freertos/FreeRTOS.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_malloc.h>

void * csp_malloc(size_t size) {
    return pvPortMalloc(size);
}

void csp_free(void *ptr) {
    vPortFree(ptr);
}
