/****************************************************************************
 * **File:** csp/arch/csp_queue.h
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

#if (CSP_FREERTOS)
typedef QueueHandle_t csp_queue_handle_t;
typedef StaticQueue_t csp_static_queue_t;
#elif (CSP_ZEPHYR)
#include <zephyr/kernel.h>
typedef struct k_msgq * csp_queue_handle_t;
typedef struct k_msgq csp_static_queue_t;
#else
typedef struct pthread_queue_s pthread_queue_t; // Opaque pointer
typedef pthread_queue_t * csp_queue_handle_t;
typedef void * csp_static_queue_t;
#endif

/**
 * Create static queue.
 *
 * @param[in] length Number of items in static queue.
 * @param[in] item_size Size of each item in static queue.
 * @param[in] buffer Memory buffer that will hold the static queue items.
                     Must be at least `length * item_size` bytes in size.
 * @param[in] queue Pointer to static queue.
 * @return Static queue handle on success, otherwise NULL.
 */
csp_queue_handle_t csp_queue_create_static(int length, size_t item_size, char * buffer, csp_static_queue_t * queue);

/**
 * Enqueue (back) value.
 *
 * @param[in] handle queue.
 * @param[in] value value to add (by copy)
 * @param[in] timeout timeout, time to wait for free space
 * @return #CSP_QUEUE_OK on success, otherwise a queue error code.
 */
int csp_queue_enqueue(csp_queue_handle_t handle, const void *value, uint32_t timeout);

/**
 * Enqueue (back) value from ISR.
 *
 * @param[in] handle queue.
 * @param[in] value value to add (by copy)
 * @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 * @return #CSP_QUEUE_OK on success, otherwise a queue error code.
 */
int csp_queue_enqueue_isr(csp_queue_handle_t handle, const void * value, int * pxTaskWoken);

/**
 * Dequeue value (front).
 *
 * @param[in] handle queue.
 * @param[out] buf extracted element (by copy).
 * @param[in] timeout timeout, time to wait for element in queue.
 * @return #CSP_QUEUE_OK on success, otherwise a queue error code.
 */
int csp_queue_dequeue(csp_queue_handle_t handle, void *buf, uint32_t timeout);

/**
 * Dequeue value (front) from ISR.
 *
 * @param[in] handle queue.
 * @param[out] buf extracted element (by copy).
 * @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 * @return #CSP_QUEUE_OK on success, otherwise a queue error code.
 */
int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, int * pxTaskWoken);

/**
 * Queue size.
 *
 * @param[in] handle handle queue.
 * @return Number of elements in the queue.
 */
int csp_queue_size(csp_queue_handle_t handle);

/**
 * Queue size from ISR.
 *
 * @param[in] handle handle queue.
 * @return Number of elements in the queue.
 */
int csp_queue_size_isr(csp_queue_handle_t handle);

/**
 * Free queue object (handle).
 *
 * @param[in] handle handle queue.
 */
int csp_queue_free(csp_queue_handle_t handle);

/**
 * Empty queue object by removing all items (handle).
 *
 * @param[in] handle handle queue.
 */
void csp_queue_empty(csp_queue_handle_t handle);

#ifdef __cplusplus
}
#endif
