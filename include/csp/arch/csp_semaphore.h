

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

typedef sem_t csp_bin_sem_handle_t;
typedef pthread_mutex_t csp_mutex_t;
typedef void * csp_bin_sem_t;          // These are not used (static allocation)
typedef void * csp_mutex_buffer_t;     // These are not used (static allocation)

#endif // CSP_POSIX

/* MAC OS X interface */
#if (CSP_MACOSX)

#include <pthread.h>
#include "posix/pthread_queue.h"

#define CSP_SEMAPHORE_OK 	PTHREAD_QUEUE_OK
#define CSP_SEMAPHORE_ERROR 	PTHREAD_QUEUE_EMPTY

typedef pthread_queue_t * csp_bin_sem_handle_t;
typedef pthread_queue_t * csp_mutex_t;

#endif // CSP_MACOSX

#if (CSP_WINDOWS)

#include <Windows.h>

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR	2

typedef HANDLE csp_bin_sem_handle_t;
typedef HANDLE csp_mutex_t;

#endif // CSP_WINDOWS

/* FreeRTOS interface */
#if (CSP_FREERTOS)

#include <FreeRTOS.h>
#include <semphr.h>

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR	0

typedef void * csp_bin_sem_handle_t;
typedef StaticSemaphore_t csp_bin_sem_t;
typedef void * csp_mutex_t;
typedef StaticSemaphore_t csp_mutex_buffer_t;

#endif // CSP_FREERTOS

/* Zephyr RTOS Interface */
#if (CSP_ZEPHYR)

#include <zephyr.h>

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR	2

typedef struct k_mutex csp_mutex_t;
typedef struct k_sem csp_bin_sem_handle_t;

/* These types are not used for static API */
struct csp_empty_t {};
typedef struct csp_empty_t csp_mutex_buffer_t;
typedef struct csp_empty_t csp_bin_sem_t;

#endif // CSP_ZEPHYR

/**
   Mutex call OK.

   @note Value is different from standard CSP error codes, see #CSP_SEMAPHORE_OK
*/
#define CSP_MUTEX_OK 		CSP_SEMAPHORE_OK

/**
   Mutex call failed.

   @note Value is different from standard CSP error codes, see #CSP_SEMAPHORE_ERROR
*/
#define CSP_MUTEX_ERROR		CSP_SEMAPHORE_ERROR

/**
   Create/initialize a mutex.

   @param[in] mutex mutex.
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_mutex_create(csp_mutex_t * mutex);

/**
   Initialize a mutex with static allocation

   @param[in] mutex mutex handle pointer
   @param buffer pointer to mutex storage
*/
void csp_mutex_create_static(csp_mutex_t * handle, csp_mutex_buffer_t * buffer);

/**
   Free a mutex.

   @param[in] mutex mutex.
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_mutex_remove(csp_mutex_t * mutex);

/**
   Lock mutex.

   @param[in] mutex mutex
   @param[in] timeout timeout in mS. Use #CSP_MAX_TIMEOUT for no timeout, e.g. wait forever until locked.
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_mutex_lock(csp_mutex_t * mutex, uint32_t timeout);

/**
   Unlock mutex.

   @param[in] mutex mutex
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_mutex_unlock(csp_mutex_t * mutex);

/**
   Create/initialize a binary semaphore.

   The semaphore is created in state \a unlocked (value 1).

   On platforms supporting max values, the semaphore is created with a max value of 1, hence the naming \a binary.

   @param[in] sem semaphore
   @return #CSP_SEMAPHORE_OK on success, otherwise #CSP_SEMAPHORE_ERROR
*/
int csp_bin_sem_create(csp_bin_sem_handle_t * sem);

/**
   initialize a binary semaphore with static storage

   The semaphore is created in state \a unlocked (value 1).

   On platforms supporting max values, the semaphore is created with a max value of 1, hence the naming \a binary.

   @param[in] sem semaphore
   @param buffer csp_bin_sem_t storage
*/
void csp_bin_sem_create_static(csp_bin_sem_handle_t * handle, csp_bin_sem_t * buffer);

/**
   Free a semaphore.

   @param[in] sem semaphore.
   @return #CSP_SEMAPHORE_OK on success, otherwise #CSP_SEMAPHORE_ERROR
*/
int csp_bin_sem_remove(csp_bin_sem_handle_t * sem);

/**
   Wait/lock semaphore.

   @param[in] sem semaphore
   @param[in] timeout timeout in mS. Use #CSP_MAX_TIMEOUT for no timeout, e.g. wait forever until locked.
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout);

/**
   Signal/unlock semaphore.

   @param[in] sem semaphore
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_bin_sem_post(csp_bin_sem_handle_t * sem);

/**
   Signal/unlock semaphore from an ISR context.

   @param[in] sem semaphore
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR*
*/
int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, int * pxTaskWoken);

