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

#include <pthread.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_queue.h>
#include <csp/csp_pthread_queue.h>

csp_queue_handle_t csp_queue_create(int length, size_t item_size) {
    return pthread_queue_create(length, item_size);
}

int csp_queue_enqueue(csp_queue_handle_t handle, void *value, int timeout) {
    return pthread_queue_enqueue(handle, value, timeout);
}

int csp_queue_enqueue_isr(csp_queue_handle_t handle, void * value, signed CSP_BASE_TYPE * task_woken) {
    task_woken = 0;
    return csp_queue_enqueue(handle, value, 0);
}

int csp_queue_dequeue(csp_queue_handle_t handle, void *buf, int timeout) {
    return pthread_queue_dequeue(handle, buf, timeout);
}

int csp_queue_dequeue_isr(csp_queue_handle_t handle, void *buf, signed CSP_BASE_TYPE * task_woken) {
    task_woken = 0;
    return csp_queue_dequeue(handle, buf, 0);
}

int csp_queue_size(csp_queue_handle_t handle) {
    return pthread_queue_items(handle);
}

int csp_queue_size_isr(csp_queue_handle_t handle) {
    return pthread_queue_items(handle);
}
