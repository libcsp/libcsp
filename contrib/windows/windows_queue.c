

#include "windows_queue.h"
#include <windows.h>
#include <synchapi.h>

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

static int queueFull(windows_queue_t * queue) {
	return queue->items == queue->size;
}

static int queueEmpty(windows_queue_t * queue) {
	return queue->items == 0;
}

windows_queue_t * windows_queue_create(int length, size_t item_size) {

	windows_queue_t * queue = (windows_queue_t *)malloc(sizeof(windows_queue_t));
	if (queue == NULL)
		goto queue_malloc_failed;

	queue->buffer = malloc(length * item_size);
	if (queue->buffer == NULL)
		goto buffer_malloc_failed;

	queue->size = length;
	queue->item_size = item_size;
	queue->items = 0;
	queue->head_idx = 0;

	InitializeCriticalSection(&(queue->mutex));
	InitializeConditionVariable(&(queue->cond_full));
	InitializeConditionVariable(&(queue->cond_empty));
	goto queue_init_success;

buffer_malloc_failed:
	free(queue);
	queue = NULL;
queue_malloc_failed:
queue_init_success:
	return queue;
}

void windows_queue_delete(windows_queue_t * q) {

	if (q == NULL) return;
	DeleteCriticalSection(&(q->mutex));
	free(q->buffer);
	free(q);
}

int windows_queue_enqueue(windows_queue_t * queue, const void * value, int timeout) {

	int offset;
	EnterCriticalSection(&(queue->mutex));
	while (queueFull(queue)) {
		int ret = SleepConditionVariableCS(&(queue->cond_full), &(queue->mutex), timeout);
		if (!ret) {
			LeaveCriticalSection(&(queue->mutex));
			return ret == WAIT_TIMEOUT ? WINDOWS_QUEUE_FULL : WINDOWS_QUEUE_ERROR;
		}
	}
	offset = ((queue->head_idx + queue->items) % queue->size) * queue->item_size;
	memcpy((unsigned char *)queue->buffer + offset, value, queue->item_size);
	queue->items++;

	LeaveCriticalSection(&(queue->mutex));
	WakeAllConditionVariable(&(queue->cond_empty));
	return WINDOWS_QUEUE_OK;
}

int windows_queue_dequeue(windows_queue_t * queue, void * buf, int timeout) {

	EnterCriticalSection(&(queue->mutex));
	while (queueEmpty(queue)) {
		int ret = SleepConditionVariableCS(&(queue->cond_empty), &(queue->mutex), timeout);
		if (!ret) {
			LeaveCriticalSection(&(queue->mutex));
			return ret == WAIT_TIMEOUT ? WINDOWS_QUEUE_EMPTY : WINDOWS_QUEUE_ERROR;
		}
	}
	memcpy(buf, (unsigned char *)queue->buffer + (queue->head_idx % queue->size * queue->item_size), queue->item_size);
	queue->items--;
	queue->head_idx = (queue->head_idx + 1) % queue->size;

	LeaveCriticalSection(&(queue->mutex));
	WakeAllConditionVariable(&(queue->cond_full));
	return WINDOWS_QUEUE_OK;
}

int windows_queue_items(windows_queue_t * queue) {

	int items;
	EnterCriticalSection(&(queue->mutex));
	items = queue->items;
	LeaveCriticalSection(&(queue->mutex));

	return items;
}
