

/*
Inspired by c-pthread-queue by Matthew Dickinson
http://code.google.com/p/c-pthread-queue/
*/

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mach/clock.h>
#include <mach/mach.h>

#include <csp/arch/posix/pthread_queue.h>

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
			if (pthread_mutex_init(&(q->mutex), NULL) || pthread_cond_init(&(q->cond_full), NULL) || pthread_cond_init(&(q->cond_empty), NULL)) {
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

int pthread_queue_enqueue(pthread_queue_t * queue, const void * value, uint32_t timeout) {

	int ret;

	/* Calculate timeout */
	struct timespec ts;

	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts.tv_sec = mts.tv_sec;
	ts.tv_nsec = mts.tv_nsec;

	uint32_t sec = timeout / 1000;
	uint32_t nsec = (timeout - 1000 * sec) * 1000000;

	ts.tv_sec += sec;

	if (ts.tv_nsec + nsec > 1000000000)
		ts.tv_sec++;

	ts.tv_nsec = (ts.tv_nsec + nsec) % 1000000000;

	/* Get queue lock */
	pthread_mutex_lock(&(queue->mutex));
	while (queue->items == queue->size) {
		ret = pthread_cond_timedwait(&(queue->cond_full), &(queue->mutex), &ts);
		if (ret != 0) {
			pthread_mutex_unlock(&(queue->mutex));
			return PTHREAD_QUEUE_FULL;
		}
	}

	/* Copy object from input buffer */
	memcpy(queue->buffer + (queue->in * queue->item_size), value, queue->item_size);
	queue->items++;
	queue->in = (queue->in + 1) % queue->size;
	pthread_mutex_unlock(&(queue->mutex));

	/* Nofify blocked threads */
	pthread_cond_broadcast(&(queue->cond_empty));

	return PTHREAD_QUEUE_OK;
}

int pthread_queue_dequeue(pthread_queue_t * queue, void * buf, uint32_t timeout) {

	int ret;

	/* Calculate timeout */
	struct timespec ts;
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts.tv_sec = mts.tv_sec;
	ts.tv_nsec = mts.tv_nsec;

	uint32_t sec = timeout / 1000;
	uint32_t nsec = (timeout - 1000 * sec) * 1000000;

	ts.tv_sec += sec;

	if (ts.tv_nsec + nsec > 1000000000)
		ts.tv_sec++;

	ts.tv_nsec = (ts.tv_nsec + nsec) % 1000000000;

	/* Get queue lock */
	pthread_mutex_lock(&(queue->mutex));
	while (queue->items == 0) {
		ret = pthread_cond_timedwait(&(queue->cond_empty), &(queue->mutex), &ts);
		if (ret != 0) {
			pthread_mutex_unlock(&(queue->mutex));
			return PTHREAD_QUEUE_EMPTY;
		}
	}

	/* Copy object to output buffer */
	memcpy(buf, queue->buffer + (queue->out * queue->item_size), queue->item_size);
	queue->items--;
	queue->out = (queue->out + 1) % queue->size;
	pthread_mutex_unlock(&(queue->mutex));

	/* Nofify blocked threads */
	pthread_cond_broadcast(&(queue->cond_full));

	return PTHREAD_QUEUE_OK;
}

int pthread_queue_items(pthread_queue_t * queue) {

	pthread_mutex_lock(&(queue->mutex));
	int items = queue->items;
	pthread_mutex_unlock(&(queue->mutex));

	return items;
}
