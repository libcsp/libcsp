

/*
Inspired by c-pthread-queue by Matthew Dickinson
http://code.google.com/p/c-pthread-queue/
*/

#include "pthread_queue.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <csp/csp.h>

static inline int get_deadline(struct timespec * ts, uint32_t timeout_ms) {
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

static inline int init_cond_clock_monotonic(pthread_cond_t * cond) {

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

	pthread_queue_t * q = malloc(sizeof(pthread_queue_t));

	if (q != NULL) {
		q->buffer = malloc(length * item_size);
		if (q->buffer != NULL) {
			q->size = length;
			q->item_size = item_size;
			q->items = 0;
			q->in = 0;
			q->out = 0;
			if (pthread_mutex_init(&(q->mutex), NULL) || init_cond_clock_monotonic(&(q->cond_full)) || init_cond_clock_monotonic(&(q->cond_empty))) {
				free(q->buffer);
				free(q);
				q = NULL;
			}
		} else {
			free(q);
			q = NULL;
		}
	}

	return q;
}

void pthread_queue_delete(pthread_queue_t * q) {

	if (q == NULL)
		return;

	free(q->buffer);
	free(q);

	return;
}

static inline int wait_slot_available(pthread_queue_t * queue, struct timespec * ts) {

	int ret;

	while (queue->items == queue->size) {

		if (ts != NULL) {
			ret = pthread_cond_timedwait(&(queue->cond_full), &(queue->mutex), ts);
		} else {
			ret = pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
		}

		if (ret != 0 && ret != EINTR) {
			return PTHREAD_QUEUE_FULL;  // Timeout
		}
	}

	return PTHREAD_QUEUE_OK;
}

int pthread_queue_enqueue(pthread_queue_t * queue, const void * value, uint32_t timeout) {

	int ret;
	struct timespec ts;
	struct timespec * pts = NULL;

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
		memcpy((char *)queue->buffer + (queue->in * queue->item_size), value, queue->item_size);
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

static inline int wait_item_available(pthread_queue_t * queue, struct timespec * ts) {

	int ret;

	while (queue->items == 0) {

		if (ts != NULL) {
			ret = pthread_cond_timedwait(&(queue->cond_empty), &(queue->mutex), ts);
		} else {
			ret = pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
		}

		if (ret != 0 && ret != EINTR) {
			return PTHREAD_QUEUE_EMPTY;  // Timeout
		}
	}

	return PTHREAD_QUEUE_OK;
}

int pthread_queue_dequeue(pthread_queue_t * queue, void * buf, uint32_t timeout) {

	int ret;
	struct timespec ts;
	struct timespec * pts;

	if(!queue){
		csp_print("csp not initialized\n");
		return PTHREAD_QUEUE_ERROR;
	}

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
		memcpy(buf, (char *)queue->buffer + (queue->out * queue->item_size), queue->item_size);
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

int pthread_queue_free(pthread_queue_t * queue) {

	pthread_mutex_lock(&(queue->mutex));
	int free = (queue->size / queue->item_size) - queue->items;
	pthread_mutex_unlock(&(queue->mutex));

	return free;
}

void pthread_queue_empty(pthread_queue_t * queue) {

	pthread_mutex_lock(&(queue->mutex));
	queue->items = 0;
	queue->in = 0;
	queue->out = 0;
	pthread_mutex_unlock(&(queue->mutex));
}
