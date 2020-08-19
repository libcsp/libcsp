/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 Gomspace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk) 

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_malloc.h>
#include <csp/csp_debug.h>

#include <string.h>

#include "mutex.h"
#include "xtimer.h"
#include "cond.h"

typedef struct {
	uint8_t *buffer;
	int size;
	int max_size;
	int item_size;
	int in;
	int out;
	mutex_t mutex;
	cond_t cond_empty;
} queue_t;

csp_queue_handle_t csp_queue_create(int length, size_t item_size) {
	queue_t *q = csp_malloc(sizeof(queue_t));

	q->buffer = csp_malloc(item_size * length);
	q->size = 0;
	q->max_size = length;
	q->item_size = item_size;
	q->in = 0;
	q->out = 0;
	mutex_init(&q->mutex);
	cond_init(&q->cond_empty);
	return q;
}

void csp_queue_remove(csp_queue_handle_t queue) {
	queue_t *q = (queue_t *)queue;
	csp_free(q->buffer);
	csp_free(q);
}

int csp_queue_enqueue(csp_queue_handle_t queue, const void * value, uint32_t timeout) {
	(void)timeout; //this function does not implement the timeout, but it is not needed
	
	queue_t *q = (queue_t *)queue;
	if(q->size >= q->max_size) {
		return CSP_QUEUE_FULL;
	}
	mutex_lock(&q->mutex);
	memcpy(q->buffer + (q->in * q->item_size), value, q->item_size);
	q->in = (q->in + 1) % q->max_size;
	q->size++;

	cond_signal(&q->cond_empty);
	mutex_unlock(&q->mutex);

	return CSP_QUEUE_OK;
}

int csp_queue_enqueue_isr(csp_queue_handle_t handle, const void * value, CSP_BASE_TYPE * task_woken) {
	return csp_queue_enqueue(handle, value, 0);
}

static void dequeue_timeout(void *arg) {
	queue_t *q = (queue_t *)arg;
	cond_signal(&q->cond_empty);
}

int csp_queue_dequeue(csp_queue_handle_t queue, void * buf, uint32_t timeout) {
	queue_t *q = (queue_t *)queue;
	mutex_lock(&q->mutex);

	if(q->size == 0 && timeout == 0) {
		mutex_unlock(&q->mutex);
		return CSP_QUEUE_ERROR; //no wait
	}
	xtimer_t timer;
	if(q->size == 0) { //empty queue
		timer.callback = dequeue_timeout;
		timer.arg = (void *)q;
		xtimer_set(&timer, timeout * 1000);

		cond_wait(&q->cond_empty, &q->mutex);

		if(q->size == 0) {
			//we woke up from the timeout
			mutex_unlock(&q->mutex);
			return CSP_QUEUE_ERROR;
		} 
		xtimer_remove(&timer);
	}


	memcpy(buf, q->buffer + (q->out * q->item_size), q->item_size);
	q->out = (q->out + 1) % q->max_size;
	q->size--;
	
	mutex_unlock(&q->mutex);

	return CSP_QUEUE_OK;
}

int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, CSP_BASE_TYPE * task_woken) {
	return csp_queue_dequeue(handle, buf, 0);
}

int csp_queue_size(csp_queue_handle_t queue) {
	queue_t *q = (queue_t *)queue;
	return q->size;
}

int csp_queue_size_isr(csp_queue_handle_t handle) {
	return csp_queue_size(handle);
}
