

#include <csp/arch/csp_queue.h>
#include <csp/csp.h>

#include <FreeRTOS.h>
#include <queue.h>

#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
csp_queue_handle_t csp_queue_create(int length, size_t item_size) {
	return xQueueCreate(length, item_size);
}

void csp_queue_remove(csp_queue_handle_t queue) {
	vQueueDelete(queue);
}
#endif

csp_queue_handle_t csp_queue_create_static(int length, size_t item_size, char * buffer, csp_static_queue_t * queue) {
	return xQueueCreateStatic(length, item_size, (uint8_t *)buffer, queue);
}

int csp_queue_enqueue(csp_queue_handle_t handle, const void * value, uint32_t timeout) {
	if (timeout != CSP_MAX_TIMEOUT)
		timeout = timeout / portTICK_PERIOD_MS;
	return xQueueSendToBack(handle, value, timeout);
}

int csp_queue_enqueue_isr(csp_queue_handle_t handle, const void * value, int * task_woken) {
	return xQueueSendToBackFromISR(handle, value, (portBASE_TYPE *)task_woken);
}

int csp_queue_dequeue(csp_queue_handle_t handle, void * buf, uint32_t timeout) {
	if (timeout != CSP_MAX_TIMEOUT)
		timeout = timeout / portTICK_PERIOD_MS;
	return xQueueReceive(handle, buf, timeout);
}

int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, int * task_woken) {
	return xQueueReceiveFromISR(handle, buf, (portBASE_TYPE *)task_woken);
}

int csp_queue_size(csp_queue_handle_t handle) {
	return uxQueueMessagesWaiting(handle);
}

int csp_queue_size_isr(csp_queue_handle_t handle) {
	return uxQueueMessagesWaitingFromISR(handle);
}

int csp_queue_free(csp_queue_handle_t handle) {
	return uxQueueSpacesAvailable(handle);
}
