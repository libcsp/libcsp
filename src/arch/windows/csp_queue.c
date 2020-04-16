/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
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

#include <csp/arch/csp_queue.h>
#include "windows_queue.h"

csp_queue_handle_t csp_queue_create(int length, size_t item_size) {
	return windows_queue_create(length, item_size);
}

void csp_queue_remove(csp_queue_handle_t queue) {
	windows_queue_delete(queue);
}

int csp_queue_enqueue(csp_queue_handle_t handle, const void *value, uint32_t timeout) {
	return windows_queue_enqueue(handle, value, timeout);
}

int csp_queue_enqueue_isr(csp_queue_handle_t handle, const void * value, CSP_BASE_TYPE * task_woken) {
	if( task_woken != NULL )
		*task_woken = 0;
	return windows_queue_enqueue(handle, value, 0);
}

int csp_queue_dequeue(csp_queue_handle_t handle, void *buf, uint32_t timeout) {
	return windows_queue_dequeue(handle, buf, timeout);
}

int csp_queue_dequeue_isr(csp_queue_handle_t handle, void * buf, CSP_BASE_TYPE * task_woken) {
	if (task_woken != NULL) {
		*task_woken = 0;
	}
	return windows_queue_dequeue(handle, buf, 0);
}

int csp_queue_size(csp_queue_handle_t handle) {
	return windows_queue_items(handle);
}

int csp_queue_size_isr(csp_queue_handle_t handle) {
	return windows_queue_items(handle);
}
