#include <stdint.h>

#ifndef _CSP_THREAD_H_
#define _CSP_THREAD_H_

/* Blackfin/x86 on Linux */
#if defined(__i386__) || defined(__BFIN__)

#include <pthread.h>

#define csp_thread_exit() pthread_exit(NULL)

typedef pthread_t csp_thread_handle_t;
typedef void* csp_thread_return_t;

#endif // __i386__ or __BFIN__

/* AVR/ARM on FreeRTOS */
#if defined(__AVR__) || defined(__ARM__)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define csp_thread_exit() return

typedef xTaskHandle csp_thread_handle_t;
typedef void csp_thread_return_t;

#endif // __AVR__ or __ARM__

int csp_thread_create(csp_thread_return_t (* routine)(void *), const signed char * const thread_name, unsigned short stack_depth, void * parameters, unsigned int priority, csp_thread_handle_t * handle);

#endif // _CSP_THREAD_H_
