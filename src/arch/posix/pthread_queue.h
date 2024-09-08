#pragma once

/**
   @file

   Queue implemented using pthread locks and conds.

   Inspired by c-pthread-queue by Matthew Dickinson: http://code.google.com/p/c-pthread-queue/
*/

#include <stdint.h>
#include <pthread.h>

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


int pthread_queue_free(pthread_queue_t * queue);

void pthread_queue_empty(pthread_queue_t * queue);
