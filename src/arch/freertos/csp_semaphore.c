

#include <FreeRTOS.h>
#include <semphr.h>

#include <csp/arch/csp_semaphore.h>
#include <csp/csp_debug.h>
#include <csp/csp.h>

void csp_bin_sem_init(csp_bin_sem_t * sem) {
	xSemaphoreCreateBinaryStatic((StaticQueue_t *) sem);
	xSemaphoreGive((QueueHandle_t) sem);
}

int csp_bin_sem_wait(csp_bin_sem_t * sem, unsigned int timeout) {
	csp_log_lock("Wait: %p", sem);
	if (timeout != CSP_MAX_TIMEOUT) {
		timeout = timeout / portTICK_PERIOD_MS;
	}
	if (xSemaphoreTake((QueueHandle_t) sem, timeout) == pdPASS) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_post(csp_bin_sem_t * sem) {
	csp_log_lock("Post: %p", sem);
	if (xSemaphoreGive((QueueHandle_t) sem) == pdPASS) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;
}