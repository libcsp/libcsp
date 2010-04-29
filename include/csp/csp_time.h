#include <stdint.h>

#include <csp/csp.h>

#ifndef _CSP_TIME_H_
#define _CSP_TIME_H_

/* Blackfin/x86 on Linux */
#if defined(__i386__) || defined(__BFIN__)

#include <time.h>
#include <sys/time.h>
#include <limits.h>

#define CSP_MAX_DELAY INT_MAX

#endif // __i386__ or __BFIN__

/* AVR/ARM on FreeRTOS */
#if defined(__AVR__) || defined(__ARM__)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define CSP_MAX_DELAY portMAX_DELAY

#endif // __AVR__ or __ARM__

uint32_t csp_get_ms();

#endif // _CSP_TIME_H_
