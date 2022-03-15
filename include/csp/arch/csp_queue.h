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

#ifndef _CSP_ARCH_QUEUE_H_
#define _CSP_ARCH_QUEUE_H_

/**
   @file

   Queue interface.
*/

#include <csp/csp_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   No error.
   @note Value is 1!
*/
#define CSP_QUEUE_OK 1

/**
   Queue full.
   @note Value is 0!
*/
#define CSP_QUEUE_FULL 0

/**
   Queue error.
   @note Value is 0!
*/
#define CSP_QUEUE_ERROR 0

/**
   Queue handle.
*/
typedef void * csp_queue_handle_t;

/**
   Create queue.
   @param[in] length max length of queue, number of elements.
   @param[in] item_size size of queue elements (bytes).
   @return Create queue on success, otherwise NULL.
*/
csp_queue_handle_t csp_queue_create(int length, size_t item_size);

/**
   Remove/delete queue.
   @param[in] queue queue.
*/
void csp_queue_remove(csp_queue_handle_t queue);

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
int csp_queue_enqueue_isr(csp_queue_handle_t handle, const void * value, CSP_BASE_TYPE * pxTaskWoken);

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
int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, CSP_BASE_TYPE * pxTaskWoken);

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

#ifdef __cplusplus
}
#endif
#endif
