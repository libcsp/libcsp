#pragma once

/**
   @file

   Thread (task) interface.
*/

#include <csp/csp_types.h>

#if (CSP_POSIX || CSP_MACOSX || __DOXYGEN__)
#include <pthread.h>

#elif (CSP_WINDOWS)
#include <windows.h>
int csp_windows_thread_create(unsigned int (*func)(void *), const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, HANDLE * handle);

#elif (CSP_FREERTOS)
// #include <FreeRTOS.h>
// #include <task.h>
int csp_freertos_thread_create(TaskFunction_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, void ** handle);
#endif

/**
   Sleep X mS.
   @param[in] time_ms mS to sleep.
*/
void csp_sleep_ms(unsigned int time_ms);
