

#pragma once

#include <csp_autoconfig.h>

#define CSP_SEMAPHORE_OK 	0
#define CSP_SEMAPHORE_ERROR	-1

/**
 * Define the size of a semaphore:
 *
 * This varies a little across different platforms and implementations
 * Here we include the platform header in order to get that size.
 * Except for POSIX which are pretty stable already
 * The FreeRTOS size depends on some compile time options, so the most
 * efficient way is to use the sizeof().
 * However we may consider getting rid of the dependency on freertos just
 * for this single number
 * Maybe we should leave this as a compile time configuration parameter
 *
 */

#if (CSP_POSIX || __DOXYGEN__)

    /* POSIX interface */
    #define CSP_SIZEOF_SEM_T 32

#elif (CSP_FREERTOS)

    /* FreeRTOS interface */
#if 0
    #include <FreeRTOS.h>
    #include <semphr.h>
    #define CSP_SIZEOF_SEM_T sizeof(StaticSemaphore_t)
#else
    #define CSP_SIZEOF_SEM_T sizeof(void *)
#endif

#elif (CSP_ZEPHYR)

    /* Zephyr RTOS Interface */
    #include <zephyr.h>
    #define CSP_SIZEOF_SEM_T sizeof(struct k_sem)

#endif

/**
 * This definition is borrowed from POSIX sem_t
 * It ensures the proper amount of memory to hold a static semaphore
 * as well as alignment is correct for the given platform.
 * The fields are never actually used directly so they can have any name
 */
typedef union {
  char __size[CSP_SIZEOF_SEM_T];
  long int __align;
} csp_bin_sem_t;

/**
 * initialize a binary semaphore with static storage
 * The semaphore is created in state \a unlocked (value 1).
 * On platforms supporting max values, the semaphore is created with a max value of 1, hence the naming \a binary.
 */
void csp_bin_sem_init(csp_bin_sem_t * sem);

/**
 * Wait/lock semaphore
 * @param[in] timeout timeout in mS. Use #CSP_MAX_TIMEOUT for no timeout, e.g. wait forever until locked.
 * @return #CSP_SEMAPHORE_OK on success, otherwise #CSP_SEMAPHORE_ERROR
 */
int csp_bin_sem_wait(csp_bin_sem_t * sem, unsigned int timeout);

/**
 * Signal/unlock semaphore
 * @return #CSP_SEMAPHORE_OK on success, otherwise #CSP_SEMAPHORE_ERROR
 */
int csp_bin_sem_post(csp_bin_sem_t * sem);
