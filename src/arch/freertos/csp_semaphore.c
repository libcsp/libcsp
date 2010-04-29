#include <stdint.h>

/* FreeRTOS includes */
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_semaphore.h>

int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {
    vSemaphoreCreateBinary(*sem);
    return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, int timeout) {
    if (xSemaphoreTake(*sem, timeout*configTICK_RATE_HZ) == pdPASS) {
        return CSP_SEMAPHORE_OK;
    } else {
        return CSP_SEMAPHORE_ERROR;
    }
}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {
    if (xSemaphoreGive(*sem) == pdPASS) {
        return CSP_SEMAPHORE_OK;
    } else {
        return CSP_SEMAPHORE_ERROR;
    }
}

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, signed char * task_woken) {
    if (xSemaphoreGiveFromISR(*sem, task_woken) == pdPASS) {
        return CSP_SEMAPHORE_OK;
    } else {
        return CSP_SEMAPHORE_ERROR;
    }
}

