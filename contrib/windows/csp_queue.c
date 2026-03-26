
#include <csp/arch/csp_queue.h>
#include <windows.h>
#include <synchapi.h>
#include <stdlib.h>
#include <string.h>

/**
 * Windows queue structure
 */
struct windows_queue_s {
	void * buffer;
	int size;
	int item_size;
	int items;
	int head_idx;
	CRITICAL_SECTION mutex;
	CONDITION_VARIABLE cond_full;
	CONDITION_VARIABLE cond_empty;
};

/**
 * Check if queue is full
 */
static int queue_full(csp_queue_handle_t queue) {
	return queue->items == queue->size;
}

/**
 * Check if queue is empty
 */
static int queue_empty(csp_queue_handle_t queue) {
	return queue->items == 0;
}

csp_queue_handle_t csp_queue_create(int length, size_t item_size) {
	csp_queue_handle_t queue = (csp_queue_handle_t)malloc(sizeof(struct windows_queue_s));
	if (queue == NULL)
		return NULL;

	queue->buffer = malloc(length * item_size);
	if (queue->buffer == NULL) {
		free(queue);
		return NULL;
	}

	queue->size = length;
	queue->item_size = item_size;
	queue->items = 0;
	queue->head_idx = 0;

	InitializeCriticalSection(&(queue->mutex));
	InitializeConditionVariable(&(queue->cond_full));
	InitializeConditionVariable(&(queue->cond_empty));

	return queue;
}

csp_queue_handle_t csp_queue_create_static(int length, size_t item_size, char * buffer, csp_static_queue_t * queue) {
	/* We ignore static allocation for windows for now */
	return csp_queue_create(length, item_size);
}

void csp_queue_remove(csp_queue_handle_t queue) {
	if (queue == NULL) return;
	DeleteCriticalSection(&(queue->mutex));
	free(queue->buffer);
	free(queue);
}

int csp_queue_enqueue(csp_queue_handle_t queue, const void * value, uint32_t timeout) {
	int offset;
	EnterCriticalSection(&(queue->mutex));
	while (queue_full(queue)) {
		int ret = SleepConditionVariableCS(&(queue->cond_full), &(queue->mutex), timeout);
		if (!ret) {
			LeaveCriticalSection(&(queue->mutex));
			return ret == WAIT_TIMEOUT ? CSP_QUEUE_ERROR : CSP_QUEUE_ERROR;
		}
	}
	offset = ((queue->head_idx + queue->items) % queue->size) * queue->item_size;
	memcpy((unsigned char *)queue->buffer + offset, value, queue->item_size);
	queue->items++;

	LeaveCriticalSection(&(queue->mutex));
	WakeAllConditionVariable(&(queue->cond_empty));
	return CSP_QUEUE_OK;
}

int csp_queue_enqueue_isr(csp_queue_handle_t queue, const void * value, int * task_woken) {
	if (task_woken != NULL)
		*task_woken = 0;
	return csp_queue_enqueue(queue, value, 0);
}

int csp_queue_dequeue(csp_queue_handle_t queue, void * buf, uint32_t timeout) {
	EnterCriticalSection(&(queue->mutex));
	while (queue_empty(queue)) {
		int ret = SleepConditionVariableCS(&(queue->cond_empty), &(queue->mutex), timeout);
		if (!ret) {
			LeaveCriticalSection(&(queue->mutex));
			return ret == WAIT_TIMEOUT ? CSP_QUEUE_ERROR : CSP_QUEUE_ERROR;
		}
	}
	memcpy(buf, (unsigned char *)queue->buffer + (queue->head_idx % queue->size * queue->item_size), queue->item_size);
	queue->items--;
	queue->head_idx = (queue->head_idx + 1) % queue->size;

	LeaveCriticalSection(&(queue->mutex));
	WakeAllConditionVariable(&(queue->cond_full));
	return CSP_QUEUE_OK;
}

int csp_queue_dequeue_isr(csp_queue_handle_t queue, void * buf, int * task_woken) {
	if (task_woken != NULL) {
		*task_woken = 0;
	}
	return csp_queue_dequeue(queue, buf, 0);
}

int csp_queue_size(csp_queue_handle_t queue) {
	int items;
	EnterCriticalSection(&(queue->mutex));
	items = queue->items;
	LeaveCriticalSection(&(queue->mutex));
	return items;
}

int csp_queue_size_isr(csp_queue_handle_t queue) {
	return csp_queue_size(queue);
}

int csp_queue_free(csp_queue_handle_t queue) {
	csp_queue_remove(queue);
	return CSP_QUEUE_OK;
}

void csp_queue_empty(csp_queue_handle_t queue) {
	if (queue == NULL) return;

	EnterCriticalSection(&(queue->mutex));
	queue->items = 0;
	queue->head_idx = 0;
	LeaveCriticalSection(&(queue->mutex));
}
