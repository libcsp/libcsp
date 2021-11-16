

#pragma once

#include <csp/csp_types.h>
/**
   @file

   Semaphore and Mutex interface.
*/



/* POSIX interface */
#if (CSP_POSIX || __DOXYGEN__)

#include <pthread.h>
#include <semaphore.h>

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR	2

typedef sem_t csp_bin_sem_t;

#endif // CSP_POSIX

/* MAC OS X interface */
#if (CSP_MACOSX)

#include <pthread.h>
#include "posix/pthread_queue.h"

#define CSP_SEMAPHORE_OK 	PTHREAD_QUEUE_OK
#define CSP_SEMAPHORE_ERROR 	PTHREAD_QUEUE_EMPTY

typedef pthread_queue_t * csp_bin_sem_handle_t;

#endif // CSP_MACOSX

#if (CSP_WINDOWS)

#include <windows.h>

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR	2

typedef HANDLE csp_bin_sem_handle_t;
typedef void * csp_bin_sem_t;

#endif // CSP_WINDOWS

/* FreeRTOS interface */
#if (CSP_FREERTOS)

#include <FreeRTOS.h>
#include <semphr.h>

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR	0

typedef void * csp_bin_sem_handle_t;
typedef StaticSemaphore_t csp_bin_sem_t;

#endif // CSP_FREERTOS

/* Zephyr RTOS Interface */
#if (CSP_ZEPHYR)

#include <zephyr.h>

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR	2

typedef struct k_sem csp_bin_sem_handle_t;

/* These types are not used for static API */
struct csp_empty_t {};
typedef struct csp_empty_t csp_mutex_buffer_t;
typedef struct csp_empty_t csp_bin_sem_t;

#endif // CSP_ZEPHYR


/**
 * initialize a binary semaphore with static storage
 * The semaphore is created in state \a unlocked (value 1).
 * On platforms supporting max values, the semaphore is created with a max value of 1, hence the naming \a binary. 
 */
void csp_bin_sem_init(csp_bin_sem_t * sem);

/**
 * Wait/lock semaphore
 * @param[in] timeout timeout in mS. Use #CSP_MAX_TIMEOUT for no timeout, e.g. wait forever until locked.
 * @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
 */
int csp_bin_sem_wait(csp_bin_sem_t * sem, uint32_t timeout);

/**
 * Signal/unlock semaphore
 * @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
 */
int csp_bin_sem_post(csp_bin_sem_t * sem);
