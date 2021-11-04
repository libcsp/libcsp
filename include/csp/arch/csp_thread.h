#pragma once

/**
   @file

   Thread (task) interface.
*/

#include <csp/csp_types.h>



/*
  POSIX interface
*/
#if (CSP_POSIX || CSP_MACOSX || __DOXYGEN__)

#include <pthread.h>

#endif // CSP_POSIX

/*
  Windows interface
*/
#if (CSP_WINDOWS)

#include <windows.h>

#endif // CSP_WINDOWS

/*
  FreeRTOS interface
*/
#if (CSP_FREERTOS)

// #include <FreeRTOS.h>
// #include <task.h>

#endif // CSP_FREERTOS

/*
  Zephyr interface
*/
#if (CSP_ZEPHYR)

#include <zephyr.h>

#endif // CSP_ZEPHYR

#if (CSP_POSIX || __DOXYGEN__)

int csp_posix_thread_create(void *(* func)(void *), const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, pthread_t * handle);
/**
   Create thread (task).

   @param[in] func thread function
   @param[in] name name of thread, supported on: FreeRTOS.
   @param[in] stack_size stack size, supported on: posix (bytes), FreeRTOS (words, word = 4 bytes).
   @param[in] parameter parameter for thread function.
   @param[in] priority thread priority, supported on: FreeRTOS.
   @param[out] handle reference to created thread.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
pthread_t
csp_posix_thread_create_static(pthread_t *new_thread, const char * const name,
							   char *stack, unsigned int stack_size,
							   void *(* func)(void *), void * parameter,
							   unsigned int priority);

void csp_posix_thread_exit(void);

/**
   Exit current thread.
   @note Not supported on all platforms.
*/
inline void csp_thread_exit(void) {
	csp_posix_thread_exit();
}

#elif (CSP_MACOSX)
int csp_macosx_thread_create(void *(*func)(void *), const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, pthread_t * handle);
void csp_macosx_thread_exit(void);
inline void csp_thread_exit(void) {
	csp_macosx_thread_exit();
}

#elif (CSP_WINDOWS)
int csp_windows_thread_create(unsigned int (*func)(void *), const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, HANDLE * handle);
void csp_windows_thread_exit(void);
inline void csp_thread_exit(void) {
	csp_windows_thread_exit();
}

#elif (CSP_FREERTOS)
int csp_freertos_thread_create(TaskFunction_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, void ** handle);
void csp_freertos_thread_exit(void);
inline void csp_thread_exit(void) {
	csp_freertos_thread_exit();
}

#elif (CSP_ZEPHYR)
k_tid_t csp_zephyr_thread_create_static(k_tid_t *new_thread, const char * const name, char *stack, unsigned int stack_size, k_thread_entry_t func, void * parameter, unsigned int priority);
#endif

/**
   Sleep X mS.
   @param[in] time_ms mS to sleep.
*/
void csp_sleep_ms(unsigned int time_ms);
