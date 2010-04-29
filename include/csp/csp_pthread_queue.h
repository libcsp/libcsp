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

#include <stdlib.h>
#include <sys/time.h>

#ifndef _PTHREAD_QUEUE_H_
#define _PTHREAD_QUEUE_H_

#define PTHREAD_QUEUE_OK 0
#define PTHREAD_QUEUE_EMPTY 1
#define PTHREAD_QUEUE_FULL 2
#define PTHREAD_QUEUE_ERROR 3

typedef struct queue {
    void *buffer;
    int capacity;
    int item_size;
    int size;
    int in;
    int out;
    pthread_mutex_t mutex;
    pthread_cond_t cond_full;
    pthread_cond_t cond_empty;
} pthread_queue_t;

pthread_queue_t * pthread_queue_create(int length, size_t item_size);
int pthread_queue_enqueue(pthread_queue_t *queue, void *value, int timeout);
int pthread_queue_dequeue(pthread_queue_t *queue, void *buf, int timeout);
int pthread_queue_size(pthread_queue_t *queue);

#endif // _PTHREAD_QUEUE_H_

