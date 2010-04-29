#include <stdint.h>

/* FreeRTOS includes */
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_queue.h>

csp_queue_handle_t csp_queue_create(int length, size_t item_size) {
    return xQueueCreate(length, item_size);
}

int csp_queue_enqueue(csp_queue_handle_t handle, void *value, int timeout) {
    return xQueueSendToBack(handle, value, timeout*configTICK_RATE_HZ);
}

int csp_queue_enqueue_isr(csp_queue_handle_t handle, void * value, CSP_BASE_TYPE * task_woken) {
    return xQueueSendToBackFromISR(handle, value, task_woken);
}

int csp_queue_dequeue(csp_queue_handle_t handle, void * buf, int timeout) {
    return xQueueReceive(handle, buf, timeout*configTICK_RATE_HZ);
}

int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, CSP_BASE_TYPE * task_woken) {
    return xQueueReceiveFromISR(handle, buf, task_woken);
}

int csp_queue_size(csp_queue_handle_t handle) {
    return uxQueueMessagesWaiting(handle);
}

int csp_queue_size_isr(csp_queue_handle_t handle) {
    return uxQueueMessagesWaitingFromISR(handle);
}
