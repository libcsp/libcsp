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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_error.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_semaphore.h>

static csp_queue_handle_t csp_buffers;
static void *csp_buffer_list;
static unsigned int count, size;

CSP_DEFINE_CRITICAL(csp_critical_lock);

int csp_buffer_init(int buf_count, int buf_size) {

	unsigned int i;
	void *element;

	count = buf_count;
	size = buf_size;

	csp_buffer_list = csp_malloc(count * size);
	if (csp_buffer_list == NULL)
		goto fail_malloc;

	csp_buffers = csp_queue_create(count, sizeof(void *));
	if (!csp_buffers)
		goto fail_queue;

	if (CSP_INIT_CRITICAL(csp_critical_lock) != CSP_ERR_NONE)
		goto fail_critical;

	memset(csp_buffer_list, 0, count * size);

	for (i = 0; i < count; i++) {
		element = csp_buffer_list + i * size;
		csp_queue_enqueue(csp_buffers, &element, 0);
	}

	return CSP_ERR_NONE;

fail_critical:
	csp_queue_remove(csp_buffers);
fail_queue:
	csp_free(csp_buffer_list);
fail_malloc:
	return CSP_ERR_NOMEM;

}

void *csp_buffer_get_isr(size_t buf_size) {
	void *buffer = NULL;
	CSP_BASE_TYPE task_woken = 0;

	if (buf_size + CSP_BUFFER_PACKET_OVERHEAD > size)
		return NULL;

	csp_queue_dequeue_isr(csp_buffers, &buffer, &task_woken);

	return buffer;
}

void *csp_buffer_get(size_t buf_size) {
	void *buffer = NULL;

	if (buf_size + CSP_BUFFER_PACKET_OVERHEAD > size) {
		csp_log_error("Attempt to allocate too large block %u\r\n", buf_size);
		return NULL;
	}

	csp_queue_dequeue(csp_buffers, &buffer, 0);

	if (buffer != NULL) {
		csp_log_buffer("BUFFER: Using element at %p\r\n", buffer);
	} else {
		csp_log_error("Out of buffers\r\n");
	}

	return buffer;
}

void csp_buffer_free_isr(void *packet) {
	CSP_BASE_TYPE task_woken = 0;
	if (!packet)
		return;
	csp_queue_enqueue_isr(csp_buffers, &packet, &task_woken);
}

void csp_buffer_free(void *packet) {
	if (!packet) {
		csp_log_error("Attempt to free null pointer\r\n");
		return;
	}
	csp_log_buffer("BUFFER: Free element at %p\r\n", packet);
	csp_queue_enqueue(csp_buffers, &packet, 0);
}

void *csp_buffer_clone(void *buffer) {

	csp_packet_t *packet = (csp_packet_t *) buffer;

	if (!packet)
		return NULL;

	csp_packet_t *clone = csp_buffer_get(packet->length);

	if (clone)
		memcpy(clone, packet, size);

	return clone;

}

int csp_buffer_remaining(void) {
	return csp_queue_size(csp_buffers);
}

int csp_buffer_size(void) {
	return size;
}
