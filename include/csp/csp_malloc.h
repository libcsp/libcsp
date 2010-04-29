#include <stdint.h>

#ifndef _CSP_MALLOC_H_
#define _CSP_MALLOC_H_

/* Blackfin/x86 on Linux */
#if defined(__i386__) || defined(__BFIN__)

#include <stdlib.h>

#endif // __i386__ or __BFIN__

/* AVR/ARM on FreeRTOS */
#if defined(__AVR__) || defined(__ARM__)

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#endif // __AVR__ or __ARM__

void * csp_malloc(size_t size);
void csp_free(void * ptr);

#endif // _CSP_MALLOC_H_
