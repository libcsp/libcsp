#include "../../csp_semaphore.h"

#include <FreeRTOS.h>
#include <semphr.h>

#include <csp/csp_debug.h>
#include <csp/csp.h>

#if 1


void csp_bin_sem_init(csp_bin_sem_t * sem) {
	*((TaskHandle_t *) sem) = NULL;
	return;
}

int csp_bin_sem_wait(csp_bin_sem_t * sem, unsigned int timeout) {

	*((TaskHandle_t *) sem) = xTaskGetCurrentTaskHandle();

	if (timeout != CSP_MAX_TIMEOUT) {
		timeout = timeout / portTICK_PERIOD_MS;
	}
	if (ulTaskNotifyTake(true, timeout) >= 1) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;
	
}

int csp_bin_sem_post(csp_bin_sem_t * sem) {

	if (*((TaskHandle_t *) sem) != NULL) {
		xTaskNotifyGive(*(TaskHandle_t *) sem);
	}
	return CSP_SEMAPHORE_OK;
}

#else

void csp_bin_sem_init(csp_bin_sem_t * sem) {
	xSemaphoreCreateBinaryStatic((StaticQueue_t *) sem);
	xSemaphoreGive((QueueHandle_t) sem);
}

int csp_bin_sem_wait(csp_bin_sem_t * sem, unsigned int timeout) {

	if (timeout != CSP_MAX_TIMEOUT) {
		timeout = timeout / portTICK_PERIOD_MS;
	}
	if (xSemaphoreTake((QueueHandle_t) sem, timeout) == pdPASS) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_post(csp_bin_sem_t * sem) {

	if (xSemaphoreGive((QueueHandle_t) sem) == pdPASS) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;
}

#endif
