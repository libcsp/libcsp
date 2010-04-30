/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 Gomspace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

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

#ifndef _CSP_QUEUE_H_
#define _CSP_QUEUE_H_

#include <stdint.h>
#include <csp/csp.h>

/* POSIX interface */
#if defined(__CSP_POSIX__)

#include <pthread.h>
#include "csp_pthread_queue.h"

#define CSP_QUEUE_EMPTY PTHREAD_QUEUE_EMPTY
#define CSP_QUEUE_FULL PTHREAD_QUEUE_FULL
#define CSP_QUEUE_OK PTHREAD_QUEUE_OK

typedef pthread_queue_t * csp_queue_handle_t;

#endif // __CSP_POSIX__

/* FreeRTOS interface */
#if defined(__CSP_FREERTOS__)

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define CSP_QUEUE_EMPTY errQUEUE_EMPTY
#define CSP_QUEUE_FULL errQUEUE_FULL
#define CSP_QUEUE_OK pdPASS

typedef xQueueHandle csp_queue_handle_t;

#endif // __CSP_FREERTOS__

csp_queue_handle_t csp_queue_create(int length, size_t item_size);
int csp_queue_enqueue(csp_queue_handle_t handle, void *value, int timeout);
int csp_queue_enqueue_isr(csp_queue_handle_t handle, void * value, signed CSP_BASE_TYPE * task_woken);
int csp_queue_dequeue(csp_queue_handle_t handle, void *buf, int timeout);
int csp_queue_dequeue_isr(csp_queue_handle_t handle, void *buf, signed CSP_BASE_TYPE * task_woken);
int csp_queue_size(csp_queue_handle_t handle);
int csp_queue_size_isr(csp_queue_handle_t handle);

#endif // _CSP_QUEUE_H_
