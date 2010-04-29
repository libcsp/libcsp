/*
c-pthread-queue - c implementation of a bounded buffer queue using posix threads
Copyright (C) 2008  Matthew Dickinson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Hacked by Jeppe Ledet-Pedersen to support FreeRTOS-like interface 
*/

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <csp/csp_pthread_queue.h>

pthread_queue_t * pthread_queue_create(int length, size_t item_size) {
    pthread_queue_t * q = malloc(sizeof(pthread_queue_t));
    
    if (q != NULL) {
	    q->buffer = malloc(length*item_size);
        if (q->buffer != NULL) {
            q->capacity = length;
            q->item_size = item_size;
            q->size = 0;
            q->in = 0;
            q->out = 0;
            if (pthread_mutex_init(&(q->mutex), NULL) || pthread_cond_init(&(q->cond_full), NULL) || pthread_cond_init(&(q->cond_empty), NULL)) {
                free(q);
                free(q->buffer);
                q = NULL;
            }
        } else {
            free(q);
            q = NULL;
        }
    }

    return q;
}
    

int pthread_queue_enqueue(pthread_queue_t *queue, void *value, int timeout) {
    int ret;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts))
        return PTHREAD_QUEUE_ERROR;
    ts.tv_sec += timeout;

    pthread_mutex_lock(&(queue->mutex));
    while (queue->size == queue->capacity) {
	    ret = pthread_cond_timedwait(&(queue->cond_full), &(queue->mutex), &ts);
	    if (ret != 0) {
	        pthread_mutex_unlock(&(queue->mutex));
	        return PTHREAD_QUEUE_FULL;
	    }
    }

    memcpy(queue->buffer+(queue->in*queue->item_size), value, queue->item_size);
    queue->size++;
    queue->in++;
    queue->in %= queue->capacity;
    pthread_mutex_unlock(&(queue->mutex));
    pthread_cond_broadcast(&(queue->cond_empty));
    return PTHREAD_QUEUE_OK;
}

int pthread_queue_dequeue(pthread_queue_t *queue, void *buf, int timeout) {
    int ret;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts))
        return PTHREAD_QUEUE_ERROR;
    ts.tv_sec += timeout;
    
    pthread_mutex_lock(&(queue->mutex));
    while (queue->size == 0) {
	    ret = pthread_cond_timedwait(&(queue->cond_empty), &(queue->mutex), &ts);
	    if (ret != 0) {
	        pthread_mutex_unlock(&(queue->mutex));
	        return PTHREAD_QUEUE_EMPTY;
	    }
    }

    memcpy(buf, queue->buffer+(queue->out*queue->item_size), queue->item_size);
    queue->size--;
    queue->out++;
    queue->out %= queue->capacity;
    pthread_mutex_unlock(&(queue->mutex));
    pthread_cond_broadcast(&(queue->cond_full));
    return PTHREAD_QUEUE_OK;
}

int pthread_queue_size(pthread_queue_t *queue) {
    pthread_mutex_lock(&(queue->mutex));
    int size = queue->size;
    pthread_mutex_unlock(&(queue->mutex));
    return size;
}
