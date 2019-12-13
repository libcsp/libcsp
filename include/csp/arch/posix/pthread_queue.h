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

#ifndef _PTHREAD_QUEUE_H_
#define _PTHREAD_QUEUE_H_

/**
   @file

   Queue implemented using pthread locks and conds.

   Inspired by c-pthread-queue by Matthew Dickinson: http://code.google.com/p/c-pthread-queue/
*/

#include <pthread.h>

#include <csp/arch/csp_queue.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Queue error codes.
   @{
*/
/**
   General error code - something went wrong.
*/
#define PTHREAD_QUEUE_ERROR CSP_QUEUE_ERROR
/**
   Queue is empty - cannot extract element.
*/
#define PTHREAD_QUEUE_EMPTY CSP_QUEUE_ERROR
/**
   Queue is full - cannot insert element.
*/
#define PTHREAD_QUEUE_FULL CSP_QUEUE_ERROR
/**
   Ok - no error.
*/
#define PTHREAD_QUEUE_OK CSP_QUEUE_OK
/** @{ */

/**
   Queue handle.
*/
typedef struct pthread_queue_s {
    //! Memory area.
    void * buffer;
    //! Memory size.
    int size;
    //! Item/element size.
    int item_size;
    //! Items/elements in queue.
    int items;
    //! Insert point.
    int in;
    //! Extract point.
    int out;
    //! Lock.
    pthread_mutex_t mutex;
    //! Wait because queue is full (insert).
    pthread_cond_t cond_full;
    //! Wait because queue is empty (extract).
    pthread_cond_t cond_empty;
} pthread_queue_t;

/**
   Create queue.
*/
pthread_queue_t * pthread_queue_create(int length, size_t item_size);

/**
   Delete queue.
*/
void pthread_queue_delete(pthread_queue_t * q);

/**
   Enqueue/insert element.
*/
int pthread_queue_enqueue(pthread_queue_t * queue, const void * value, uint32_t timeout);

/**
   Dequeue/extract element.
*/
int pthread_queue_dequeue(pthread_queue_t * queue, void * buf, uint32_t timeout);

/**
   Return number of elements in the queue.
*/
int pthread_queue_items(pthread_queue_t * queue);

#ifdef __cplusplus
}
#endif
#endif
