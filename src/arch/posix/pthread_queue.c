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

/*
Inspired by c-pthread-queue by Matthew Dickinson
http://code.google.com/p/c-pthread-queue/
*/

#include <csp/arch/posix/pthread_queue.h>

#include <errno.h>
#include <string.h>

#include <csp/arch/csp_malloc.h>

static inline int get_deadline(struct timespec *ts, uint32_t timeout_ms)
{
	int ret = clock_gettime(CLOCK_MONOTONIC, ts);

	if (ret < 0) {
		return ret;
	}

	uint32_t sec = timeout_ms / 1000;
	uint32_t nsec = (timeout_ms - 1000 * sec) * 1000000;

	ts->tv_sec += sec;

	if (ts->tv_nsec + nsec >= 1000000000) {
		ts->tv_sec++;
	}

	ts->tv_nsec = (ts->tv_nsec + nsec) % 1000000000;

	return ret;
}

static inline int init_cond_clock_monotonic(pthread_cond_t * cond)
{

	int ret;
	pthread_condattr_t attr;

	pthread_condattr_init(&attr);
	ret = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

	if (ret == 0) {
		ret = pthread_cond_init(cond, &attr);
	}

	pthread_condattr_destroy(&attr);
	return ret;

}

pthread_queue_t * pthread_queue_create(int length, size_t item_size) {
	
	pthread_queue_t * q = csp_malloc(sizeof(pthread_queue_t));
	
	if (q != NULL) {
		q->buffer = csp_malloc(length*item_size);
		if (q->buffer != NULL) {
			q->size = length;
			q->item_size = item_size;
			q->items = 0;
			q->in = 0;
			q->out = 0;
			if (pthread_mutex_init(&(q->mutex), NULL) || init_cond_clock_monotonic(&(q->cond_full)) || init_cond_clock_monotonic(&(q->cond_empty))) {
				csp_free(q->buffer);
				csp_free(q);
				q = NULL;
			}
		} else {
			csp_free(q);
			q = NULL;
		}
	}

	return q;
	
}

void pthread_queue_delete(pthread_queue_t * q) {

	if (q == NULL)
		return;

	csp_free(q->buffer);
	csp_free(q);

	return;

}
	

static inline int wait_slot_available(pthread_queue_t * queue, struct timespec *ts) {

	int ret;

	while (queue->items == queue->size) {

		if (ts != NULL) {
			ret = pthread_cond_timedwait(&(queue->cond_full), &(queue->mutex), ts);
		} else {
			ret = pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
		}

		if (ret != 0 && errno != EINTR) {
			return PTHREAD_QUEUE_FULL; //Timeout
		}
	}

	return PTHREAD_QUEUE_OK;

}

int pthread_queue_enqueue(pthread_queue_t * queue, const void * value, uint32_t timeout) {

	int ret;
	struct timespec ts;
	struct timespec *pts = NULL;

	/* Calculate timeout */
	if (timeout != CSP_MAX_TIMEOUT) {
		if (get_deadline(&ts, timeout) != 0) {
			return PTHREAD_QUEUE_ERROR;
		}
		pts = &ts;
	} else {
		pts = NULL;
	}

	/* Get queue lock */
	pthread_mutex_lock(&(queue->mutex));

	ret = wait_slot_available(queue, pts);
	if (ret == PTHREAD_QUEUE_OK) {
		/* Copy object from input buffer */
		memcpy(queue->buffer+(queue->in * queue->item_size), value, queue->item_size);
		queue->items++;
		queue->in = (queue->in + 1) % queue->size;
	}

	pthread_mutex_unlock(&(queue->mutex));

	if (ret == PTHREAD_QUEUE_OK) {
		/* Nofify blocked threads */
		pthread_cond_broadcast(&(queue->cond_empty));
	}

	return ret;

}

static inline int wait_item_available(pthread_queue_t * queue, struct timespec *ts) {

	int ret;

	while (queue->items == 0) {

		if (ts != NULL) {
			ret = pthread_cond_timedwait(&(queue->cond_empty), &(queue->mutex), ts);
		} else {
			ret = pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
		}

		if (ret != 0 && errno != EINTR) {
			return PTHREAD_QUEUE_EMPTY; //Timeout
		}
	}

	return PTHREAD_QUEUE_OK;

}

int pthread_queue_dequeue(pthread_queue_t * queue, void * buf, uint32_t timeout) {

	int ret;
	struct timespec ts;
	struct timespec *pts;

	/* Calculate timeout */
	if (timeout != CSP_MAX_TIMEOUT) {
		if (get_deadline(&ts, timeout) != 0) {
			return PTHREAD_QUEUE_ERROR;
		}
		pts = &ts;
	} else {
		pts = NULL;
	}

	/* Get queue lock */
	pthread_mutex_lock(&(queue->mutex));

	ret = wait_item_available(queue, pts);
	if (ret == PTHREAD_QUEUE_OK) {
		/* Coby object to output buffer */
		memcpy(buf, queue->buffer+(queue->out * queue->item_size), queue->item_size);
		queue->items--;
		queue->out = (queue->out + 1) % queue->size;
	}

	pthread_mutex_unlock(&(queue->mutex));

	if (ret == PTHREAD_QUEUE_OK) {
		/* Nofify blocked threads */
		pthread_cond_broadcast(&(queue->cond_full));
	}

	return ret;

}

int pthread_queue_items(pthread_queue_t * queue) {

	pthread_mutex_lock(&(queue->mutex));
	int items = queue->items;
	pthread_mutex_unlock(&(queue->mutex));
	
	return items;
	
}
