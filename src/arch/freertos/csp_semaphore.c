#include "../../csp_semaphore.h"

#include <FreeRTOS.h>
#include <semphr.h>

#include <csp/csp_debug.h>
#include <csp/csp.h>

void csp_bin_sem_init(csp_bin_sem_t * sem) {
	xSemaphoreCreateBinaryStatic(sem);
	xSemaphoreGive((SemaphoreHandle_t) sem);
}

int csp_bin_sem_wait(csp_bin_sem_t * sem, unsigned int timeout) {

	if (timeout != CSP_MAX_TIMEOUT) {
		timeout = timeout / portTICK_PERIOD_MS;
	}
	if (xSemaphoreTake((SemaphoreHandle_t) sem, timeout) == pdPASS) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_post(csp_bin_sem_t * sem) {

	/* Giving an already given binary semaphore fails, but the semaphore
	 * is in the desired state, so return OK like the POSIX and Zephyr ports */
	xSemaphoreGive((SemaphoreHandle_t) sem);
	return CSP_SEMAPHORE_OK;
}
