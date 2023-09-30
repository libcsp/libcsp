/****************************************************************************
 * **File:** arch/csp_queue.h
 *
 * **Description:** CSP queue
 ****************************************************************************/
#pragma once

#include <inttypes.h>
#include <stddef.h>
#include "csp/autoconfig.h"

#if (CSP_FREERTOS)
#include <FreeRTOS.h>
#include <queue.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CSP_QUEUE_OK 0
#define CSP_QUEUE_ERROR -1

typedef void * csp_queue_handle_t;

#if (CSP_FREERTOS)
typedef StaticQueue_t csp_static_queue_t;
#elif (CSP_ZEPHYR)
#include <zephyr/kernel.h>
typedef struct k_msgq csp_static_queue_t;
#else
typedef void * csp_static_queue_t;
#endif

csp_queue_handle_t csp_queue_create_static(int length, size_t item_size, char * buffer, csp_static_queue_t * queue);

/**
 * Enqueue (back) value.
 *
 * Parameters:
 *	handle (csp_queue_handle_t) [in]: queue.
 *	value (const void *) [in]: value to add (by copy)
 *	timeout (uint32_t) [in]: timeout, time to wait for free space
 *
 * Returns:
 *	int: #CSP_QUEUE_OK on success, otherwise a queue error code.
 */
int csp_queue_enqueue(csp_queue_handle_t handle, const void *value, uint32_t timeout);

/**
 * Enqueue (back) value from ISR.
 *
 * Parameters:
 *	handle (csp_queue_handle_t) [in]: queue.
 *	value (csp_queue_handle_t) [in]: value to add (by copy)
 *	pxTaskWoken (int *) [out]: Valid reference if called from ISR, otherwise NULL!
 *
 * Returns:
 *	int: #CSP_QUEUE_OK on success, otherwise a queue error code.
 */
int csp_queue_enqueue_isr(csp_queue_handle_t handle, const void * value, int * pxTaskWoken);

/**
 * Dequeue value (front).
 *
 * Parameters:
 *	handle (csp_queue_handle_t) [in]: queue.
 *	buf (void *) [out]: extracted element (by copy).
 *	timeout (uint32_t) [in]: timeout, time to wait for element in queue.
 *
 * Returns:
 *	int: #CSP_QUEUE_OK on success, otherwise a queue error code.
 */
int csp_queue_dequeue(csp_queue_handle_t handle, void *buf, uint32_t timeout);

/**
 * Dequeue value (front) from ISR.
 *
 * Parameters:
 *	handle (csp_queue_handle_t) [in]: queue.
 *	buf (void *) [out]: extracted element (by copy).
 *	pxTaskWoken (int *) [out]: Valid reference if called from ISR, otherwise NULL!
 *
 * Returns:
 *	int: #CSP_QUEUE_OK on success, otherwise a queue error code.
 */
int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, int * pxTaskWoken);

/**
 * Queue size.
 *
 * Parameters:
 *	handle (csp_queue_handle_t) [in]: handle queue.
 *
 * Returns:
 *	int: Number of elements in the queue.
 */
int csp_queue_size(csp_queue_handle_t handle);

/**
 * Queue size from ISR.
 *
 * Parameters:
 *	handle (csp_queue_handle_t) [in]: handle queue.
 *
 * Returns:
 *	int: Number of elements in the queue.
 */
int csp_queue_size_isr(csp_queue_handle_t handle);

/**
 * Free queue object (handle).
 *
 * Parameters:
 *	handle (csp_queue_handle_t) [in]: handle queue.
 *
 */
int csp_queue_free(csp_queue_handle_t handle);

#ifdef __cplusplus
}
#endif
