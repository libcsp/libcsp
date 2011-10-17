#include "windows_queue.h"
#include <Windows.h>

static int queueFull(windows_queue_t * queue) {
	return queue->items == queue->size;
}

static int queueEmpty(windows_queue_t * queue) {
	return queue->items == 0;
}

windows_queue_t * windows_queue_create(int length, size_t item_size) {
	windows_queue_t *queue = (windows_queue_t*)malloc(sizeof(windows_queue_t));
	if(queue == NULL)
		goto queue_malloc_failed;

	queue->buffer = malloc(length*item_size);
	if(queue->buffer == NULL)
		goto buffer_malloc_failed;

	queue->size = length;
	queue->item_size = item_size;
	queue->items = 0;
	queue->head_idx = 0;

	InitializeCriticalSection(&(queue->mutex));
	InitializeConditionVariable(&(queue->cond_full));
	InitializeConditionVariable(&(queue->cond_empty));
	queue->event_interrupt = CreateEvent(NULL, TRUE, FALSE, NULL);
	goto queue_init_success;

buffer_malloc_failed:
	free(queue);
	queue = NULL;
queue_malloc_failed:
queue_init_success:
	return queue;
}

void windows_queue_delete(windows_queue_t * q) {
	if(q==NULL) return;
	DeleteCriticalSection(&(q->mutex));
	CloseHandle(q->event_interrupt);
	free(q->buffer);
	free(q);
}

int windows_queue_enqueue(windows_queue_t * queue, void * value, int timeout) {
	int offset;
	EnterCriticalSection(&(queue->mutex));
	while(queueFull(queue)) {
		int ret = SleepConditionVariableCS(&(queue->cond_full), &(queue->mutex), timeout);
		BOOL quitSignalled = WaitForSingleObject(queue->event_interrupt, 0) == WAIT_OBJECT_0;
		if( !ret || quitSignalled ) {
			LeaveCriticalSection(&(queue->mutex));
			if( GetLastError() == WAIT_TIMEOUT )
				return WINDOWS_QUEUE_FULL;
			return WINDOWS_QUEUE_ERROR;
		}
	}
	offset = ((queue->head_idx+queue->items) % queue->size) * queue->item_size;
	memcpy((unsigned char*)queue->buffer + offset, value, queue->item_size);
	queue->items++;

	LeaveCriticalSection(&(queue->mutex));
	WakeAllConditionVariable(&(queue->cond_empty));
	return WINDOWS_QUEUE_OK;
}

int windows_queue_dequeue(windows_queue_t * queue, void * buf, int timeout) {
	EnterCriticalSection(&(queue->mutex));
	while(queueEmpty(queue)) {
		int ret = SleepConditionVariableCS(&(queue->cond_empty), &(queue->mutex), timeout);
		BOOL quitSignalled = WaitForSingleObject(queue->event_interrupt, 0) == WAIT_OBJECT_0;
		if( !ret || quitSignalled ) {
			LeaveCriticalSection(&(queue->mutex));
			if(GetLastError() == WAIT_TIMEOUT)
				return WINDOWS_QUEUE_EMPTY;
			return WINDOWS_QUEUE_ERROR;
		}
	}
	memcpy(buf, (unsigned char*)queue->buffer+(queue->head_idx%queue->size*queue->item_size), queue->item_size);
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

void windows_queue_interrupt(windows_queue_t * queue) {
	WakeAllConditionVariable(&(queue->cond_empty));
	WakeAllConditionVariable(&(queue->cond_full));
	SetEvent(queue->event_interrupt);
}
