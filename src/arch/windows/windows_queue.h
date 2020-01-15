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

#ifndef _WINDOWS_QUEUE_H_
#define _WINDOWS_QUEUE_H_

#include <csp/arch/csp_queue.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WINDOWS_QUEUE_ERROR CSP_QUEUE_ERROR
#define WINDOWS_QUEUE_EMPTY CSP_QUEUE_ERROR
#define WINDOWS_QUEUE_FULL CSP_QUEUE_ERROR
#define WINDOWS_QUEUE_OK CSP_QUEUE_OK

typedef struct windows_queue_s windows_queue_t;

windows_queue_t * windows_queue_create(int length, size_t item_size);
void windows_queue_delete(windows_queue_t * q);
int windows_queue_enqueue(windows_queue_t * queue, const void * value, int timeout);
int windows_queue_dequeue(windows_queue_t * queue, void * buf, int timeout);
int windows_queue_items(windows_queue_t * queue);

#ifdef __cplusplus
}
#endif
#endif
