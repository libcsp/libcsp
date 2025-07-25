#include "../../csp_semaphore.h"

#include <csp/csp.h>
#include <csp/csp_debug.h>

void csp_bin_sem_init(csp_bin_sem_t * sem) {
	semaphoreCreate((semaphore_t *)sem, 1);
}

int csp_bin_sem_wait(csp_bin_sem_t * sem, unsigned int timeout) {
	int ret;

	if (timeout == CSP_MAX_TIMEOUT) {
		ret = semaphoreDown(sem, 0);
	} else {
		ret = semaphoreDown(sem, timeout);
	}

	if (ret != 0)
		return CSP_SEMAPHORE_ERROR;

	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_post(csp_bin_sem_t * sem) {
	if (sem == NULL)
		return CSP_SEMAPHORE_ERROR;

	mutexLock(sem->mutex);

	condSignal(sem->cond);

    if(sem->v == 1){
        mutexUnlock(sem->mutex);
        return CSP_SEMAPHORE_OK;
    }
	++sem->v;
	mutexUnlock(sem->mutex);

	return CSP_SEMAPHORE_OK;
}
