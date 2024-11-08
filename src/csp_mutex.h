#pragma once

#if (CSP_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#elif (CSP_ZEPHYR)
#include <zephyr/kernel.h>
#else
#include <pthread.h>
#endif

typedef struct {
#if (CSP_FREERTOS)
    SemaphoreHandle_t handle; /**< FreeRTOS handle for the recursive mutex. */
    StaticQueue_t buffer;     /**< Static memory buffer for the mutex (for static allocation). */
#elif (CSP_ZEPHYR)
    struct k_mutex handle;    /**< Zephyr kernel mutex structure. */
#else
    pthread_mutex_t handle;   /**< POSIX pthread mutex structure. */
#endif
} csp_mutex_t;

/**
 * Initialize a mutex with priority inheritance and recursive locking.
 *
 * This function initializes a mutex with recursive locking enabled, allowing the same task to acquire it multiple times.
 * It also enables priority inheritance to prevent priority inversion. The implementation is platform-specific.
 *
 * @param[in,out] m Pointer to a `csp_mutex_t` structure to initialize.
 */
void csp_mutex_init(csp_mutex_t * m);

/**
 * Acquire a mutex.
 *
 * This function locks the mutex. If it is already held by another task, the calling task will block until it becomes available.
 * The same task may lock the mutex multiple times.
 *
 * @param[in,out] m Pointer to the initialized `csp_mutex_t` structure representing the mutex to lock.
 */
void csp_mutex_lock(csp_mutex_t * m);

/**
 * Release a mutex.
 *
 * This function unlocks the mutex, allowing other tasks to acquire it. If the mutex 
 * was locked multiple times by the same task, each call to `csp_mutex_unlock` will decrement the lock count until 
 * it reaches zero, at which point the mutex is fully released.
 *
 * @param[in,out] m Pointer to the initialized `csp_mutex_t` structure representing the mutex to unlock.
 */
void csp_mutex_unlock(csp_mutex_t * m);
