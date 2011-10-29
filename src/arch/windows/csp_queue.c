#include <stdint.h>
#include <csp/csp.h>
#include "../csp_queue.h"
#include "windows_queue.h"

csp_queue_handle_t csp_queue_create(int length, size_t item_size) {
	return windows_queue_create(length, item_size);
}

void csp_queue_remove(csp_queue_handle_t queue) {
	windows_queue_delete(queue);
}

int csp_queue_enqueue(csp_queue_handle_t handle, void *value, uint32_t timeout) {
	return windows_queue_enqueue(handle, value, timeout);
}

int csp_queue_enqueue_isr(csp_queue_handle_t handle, void * value, CSP_BASE_TYPE * task_woken) {
	if( task_woken != NULL )
		*task_woken = 0;
	return windows_queue_enqueue(handle, value, 0);
}

int csp_queue_dequeue(csp_queue_handle_t handle, void *buf, uint32_t timeout) {
	return windows_queue_dequeue(handle, buf, timeout);
}

int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, CSP_BASE_TYPE * task_woken) {
	if( task_woken != NULL )
		*task_woken = 0;
	return windows_queue_dequeue(handle, buf, 0);
}

int csp_queue_size(csp_queue_handle_t handle) {
	return windows_queue_items(handle);
}

int csp_queue_size_isr(csp_queue_handle_t handle) {
	return windows_queue_items(handle);
}
