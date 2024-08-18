

#include <csp/arch/csp_queue.h>
#include "pthread_queue.h"

csp_queue_handle_t csp_queue_create_static(int length, size_t item_size, char * buffer, csp_static_queue_t * queue) {
	/* We ignore static allocation for posix for now */
	return pthread_queue_create(length, item_size);
}

int csp_queue_enqueue(csp_queue_handle_t handle, const void * value, uint32_t timeout) {
	return pthread_queue_enqueue(handle, value, timeout);
}

int csp_queue_enqueue_isr(csp_queue_handle_t handle, const void * value, int * task_woken) {
	if (task_woken != NULL) {
		*task_woken = 0;
	}
	return csp_queue_enqueue(handle, value, 0);
}

int csp_queue_dequeue(csp_queue_handle_t handle, void * buf, uint32_t timeout) {
	return pthread_queue_dequeue(handle, buf, timeout);
}

int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, int * task_woken) {
	if (task_woken != NULL) {
		*task_woken = 0;
	}
	return csp_queue_dequeue(handle, buf, 0);
}

int csp_queue_size(csp_queue_handle_t handle) {
	return pthread_queue_items(handle);
}

int csp_queue_size_isr(csp_queue_handle_t handle) {
	return pthread_queue_items(handle);
}

int csp_queue_free(csp_queue_handle_t handle) {
	return pthread_queue_free(handle);
}

void csp_queue_empty(csp_queue_handle_t handle) {
	pthread_queue_empty(handle);
}
