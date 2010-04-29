#include <stdint.h>

#ifndef _CSP_SEMAPHORE_H_
#define _CSP_SEMAPHORE_H_

/* Blackfin/x86 on Linux */
#if defined(__i386__) || defined(__BFIN__)

#include <pthread.h>
#include <semaphore.h>

#define CSP_SEMAPHORE_OK 1
#define CSP_SEMAPHORE_ERROR 2

typedef sem_t csp_bin_sem_handle_t;

#endif // __i386__ or __BFIN__

/* AVR/ARM on FreeRTOS */
#if defined(__AVR__) || defined(__ARM__)

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define CSP_SEMAPHORE_OK pdPASS
#define CSP_SEMAPHORE_ERROR pdFAIL

typedef xSemaphoreHandle csp_bin_sem_handle_t;

#endif // __AVR__ or __ARM__

int csp_bin_sem_create(csp_bin_sem_handle_t *sem);
int csp_bin_sem_wait(csp_bin_sem_handle_t *sem, int timeout);
int csp_bin_sem_post(csp_bin_sem_handle_t *sem);
int csp_bin_sem_post_isr(csp_bin_sem_handle_t *sem, signed char * task_woken);

#endif // _CSP_SEMAPHORE_H_
