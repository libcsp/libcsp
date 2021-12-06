

#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <csp_autoconfig.h>

#if (CSP_FREERTOS)
#include <FreeRTOS.h>
#include <queue.h>
#endif

#define CSP_QUEUE_OK 0
#define CSP_QUEUE_ERROR -1

typedef void * csp_queue_handle_t;

#if (CSP_FREERTOS)
typedef StaticQueue_t csp_static_queue_t;
#elif (CSP_ZEPHYR)
#include <zephyr.h>
typedef struct k_msgq csp_static_queue_t;
#else
typedef void * csp_static_queue_t;
#endif

csp_queue_handle_t csp_queue_create_static(int length, size_t item_size, char * buffer, csp_static_queue_t * queue);

/**
   Enqueue (back) value.
   @param[in] handle queue.
   @param[in] value value to add (by copy)
   @param[in] timeout timeout, time to wait for free space
   @return #CSP_QUEUE_OK on success, otherwise a queue error code.
*/
int csp_queue_enqueue(csp_queue_handle_t handle, const void *value, uint32_t timeout);

/**
   Enqueue (back) value from ISR.
   @param[in] handle queue.
   @param[in] value value to add (by copy)
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
   @return #CSP_QUEUE_OK on success, otherwise a queue error code.
*/
int csp_queue_enqueue_isr(csp_queue_handle_t handle, const void * value, int * pxTaskWoken);

/**
   Dequeue value (front).
   @param[in] handle queue.
   @param[out] buf extracted element (by copy).
   @param[in] timeout timeout, time to wait for element in queue.
   @return #CSP_QUEUE_OK on success, otherwise a queue error code.
*/
int csp_queue_dequeue(csp_queue_handle_t handle, void *buf, uint32_t timeout);

/**
   Dequeue value (front) from ISR.
   @param[in] handle queue.
   @param[out] buf extracted element (by copy).
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
   @return #CSP_QUEUE_OK on success, otherwise a queue error code.
*/
int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, int * pxTaskWoken);

/**
   Queue size.
   @param[in] handle queue.
   @return Number of elements in the queue.
*/
int csp_queue_size(csp_queue_handle_t handle);

/**
   Queue size from ISR.
   @param[in] handle queue.
   @return Number of elements in the queue.
*/
int csp_queue_size_isr(csp_queue_handle_t handle);

int csp_queue_free(csp_queue_handle_t handle);

