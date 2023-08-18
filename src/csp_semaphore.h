

#pragma once

#include "csp/autoconfig.h"

#define CSP_SEMAPHORE_OK 	0
#define CSP_SEMAPHORE_ERROR	-1

#if (CSP_POSIX || __DOXYGEN__)
    #include <semaphore.h>
    typedef sem_t csp_bin_sem_t;
#elif (CSP_FREERTOS)
    #include <FreeRTOS.h>
    #include <task.h>
    typedef TaskHandle_t csp_bin_sem_t;
#elif (CSP_ZEPHYR)
    #include <zephyr/kernel.h>
    typedef struct k_sem csp_bin_sem_t;
#endif

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
