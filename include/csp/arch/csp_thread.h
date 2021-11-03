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

/**
   Platform specific thread handle.
*/
typedef pthread_t csp_thread_handle_t;
typedef pthread_t csp_thread_t;

/**
   Platform specific thread return type.
*/
typedef void * csp_thread_return_t;

/**
   Platform specific thread function.
   @param[in] parameter parameter to thread function #csp_thread_return_t.
*/
typedef csp_thread_return_t (* csp_thread_func_t)(void * parameter);

/**
   Macro for creating a thread.
*/
#define CSP_DEFINE_TASK(task_name) csp_thread_return_t task_name(void * param)

/**
   Return value for a thread function.
   Can be used as argument for normal return, eg "return CSP_TASK_RETURN";
*/
#define CSP_TASK_RETURN NULL

#endif // CSP_POSIX

/*
  Windows interface
*/
#if (CSP_WINDOWS)

#include <Windows.h>

typedef HANDLE csp_thread_handle_t;
typedef unsigned int csp_thread_return_t;
typedef csp_thread_return_t (* csp_thread_func_t)(void *) __attribute__((stdcall));

#define CSP_DEFINE_TASK(task_name) csp_thread_return_t __attribute__((stdcall)) task_name(void * param)
#define CSP_TASK_RETURN 0

#endif // CSP_WINDOWS

/*
  FreeRTOS interface
*/
#if (CSP_FREERTOS)

// #include <FreeRTOS.h>
// #include <task.h>

typedef void * csp_thread_handle_t;
typedef void csp_thread_return_t;
typedef csp_thread_return_t (* csp_thread_func_t)(void *);

#define CSP_DEFINE_TASK(task_name) csp_thread_return_t task_name(void * param)
#define CSP_TASK_RETURN

#endif // CSP_FREERTOS

/*
  Zephyr interface
*/
#if (CSP_ZEPHYR)

#include <zephyr.h>

typedef k_tid_t csp_thread_handle_t;
typedef struct k_thread csp_thread_t;
typedef k_thread_entry_t csp_thread_func_t;

#define CSP_DEFINE_TASK(task_name) void task_name(void *p1, void *p2, void *p3)
#define CSP_TASK_RETURN

#endif // CSP_ZEPHYR

#if (CSP_POSIX || __DOXYGEN__)

int csp_posix_thread_create(csp_thread_func_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, csp_thread_handle_t * handle);
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
inline int csp_thread_create(csp_thread_func_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, csp_thread_handle_t * handle) {
	return csp_posix_thread_create(func, name, stack_size, parameter, priority, handle);
}

csp_thread_handle_t
csp_posix_thread_create_static(csp_thread_handle_t *new_thread, const char * const name,
			 char *stack, unsigned int stack_size,
			 csp_thread_func_t func, void * parameter,
			 unsigned int priority);

inline csp_thread_handle_t
csp_thread_create_static(csp_thread_handle_t *new_thread, const char * const name,
			 char *stack, unsigned int stack_size,
			 csp_thread_func_t func, void * parameter,
			 unsigned int priority) {
	return csp_posix_thread_create_static(new_thread, name, stack, stack_size, func, parameter, priority);
}

void csp_posix_thread_exit(void);

/**
   Exit current thread.
   @note Not supported on all platforms.
*/
inline void csp_thread_exit(void) {
	csp_posix_thread_exit();
}

#elif (CSP_MACOSX)
int csp_macosx_thread_create(csp_thread_func_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, csp_thread_handle_t * handle);
inline int csp_thread_create(csp_thread_func_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, csp_thread_handle_t * handle) {
	return csp_macosx_thread_create(func, name, stack_size, parameter, priority, handle);
}
void csp_macosx_thread_exit(void);
inline void csp_thread_exit(void) {
	csp_macosx_thread_exit();
}

#elif (CSP_WINDOWS)
int csp_windows_thread_create(csp_thread_func_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, csp_thread_handle_t * handle);
inline int csp_thread_create(csp_thread_func_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, csp_thread_handle_t * handle) {
	return csp_windows_thread_create(func, name, stack_size, parameter, priority, handle);
}
void csp_windows_thread_exit(void);
inline void csp_thread_exit(void) {
	csp_windows_thread_exit();
}

#elif (CSP_FREERTOS)
int csp_freertos_thread_create(csp_thread_func_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, csp_thread_handle_t * handle);
inline int csp_thread_create(csp_thread_func_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, csp_thread_handle_t * handle) {
	return csp_freertos_thread_create(func, name, stack_size, parameter, priority, handle);
}
void csp_freertos_thread_exit(void);
inline void csp_thread_exit(void) {
	csp_freertos_thread_exit();
}

#elif (CSP_ZEPHYR)
csp_thread_handle_t csp_zephyr_thread_create_static(csp_thread_handle_t *new_thread, const char * const name, char *stack, unsigned int stack_size, csp_thread_func_t func, void * parameter, unsigned int priority);
inline csp_thread_handle_t csp_thread_create_static(csp_thread_handle_t *new_thread, const char * const name, char *stack, unsigned int stack_size, csp_thread_func_t func, void * parameter, unsigned int priority) {
	return csp_zephyr_thread_create_static(new_thread, name, stack, stack_size, func, parameter, priority);
}
#endif

/**
   Sleep X mS.
   @param[in] time_ms mS to sleep.
*/
void csp_sleep_ms(unsigned int time_ms);
