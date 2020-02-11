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

#include <csp/csp_buffer.h>

#include <csp/csp_debug.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_malloc.h>
#include "csp_init.h"

#ifndef CSP_BUFFER_ALIGN
#define CSP_BUFFER_ALIGN	(sizeof(int *))
#endif

/** Internal buffer header */
typedef struct csp_skbf_s {
	unsigned int refcount;
	void * skbf_addr;
	char skbf_data[]; // -> csp_packet_t
} csp_skbf_t;

// Queue of free CSP buffers
static csp_queue_handle_t csp_buffers;
// Chunk of memory allocated for CSP buffers
static char * csp_buffer_pool;

// Ensure the csp_packet is correctly aligned (as it is not packed)
CSP_STATIC_ASSERT(CSP_HEADER_LENGTH == sizeof(csp_id_t), csp_header_length);
CSP_STATIC_ASSERT(sizeof(csp_packet_t) == 16, csp_packet);
CSP_STATIC_ASSERT(offsetof(csp_packet_t, length) == 10, length_field_misaligned);
CSP_STATIC_ASSERT(offsetof(csp_packet_t, id) == 12, csp_id_field_misaligned);
CSP_STATIC_ASSERT(offsetof(csp_packet_t, data) == 16, data_field_misaligned);

int csp_buffer_init(void) {

	// calculate total size and ensure correct alignment (int *) for buffers
	const unsigned int skbfsize = CSP_BUFFER_ALIGN * ((sizeof(csp_skbf_t) + csp_buffer_size() + (CSP_BUFFER_ALIGN - 1)) / CSP_BUFFER_ALIGN);

	csp_buffer_pool = csp_malloc(csp_conf.buffers * skbfsize);
	if (csp_buffer_pool == NULL)
		goto fail_malloc;

	csp_buffers = csp_queue_create(csp_conf.buffers, sizeof(void *));
	if (!csp_buffers)
		goto fail_queue;

	for (unsigned int i = 0; i < csp_conf.buffers; i++) {
		csp_skbf_t * buf = (void *) &csp_buffer_pool[i * skbfsize];
		buf->skbf_addr = buf;
		csp_queue_enqueue(csp_buffers, &buf, 0);
	}

	return CSP_ERR_NONE;

fail_queue:
	csp_buffer_free_resources();
fail_malloc:
	return CSP_ERR_NOMEM;

}

void csp_buffer_free_resources(void) {

	if (csp_buffers) {
		csp_queue_remove(csp_buffers);
		csp_buffers = NULL;
	}
	csp_free(csp_buffer_pool);
	csp_buffer_pool = NULL;

}

void *csp_buffer_get_isr(size_t _data_size) {

	if (_data_size > csp_conf.buffer_data_size)
		return NULL;

	csp_skbf_t * buffer = NULL;
	CSP_BASE_TYPE task_woken = 0;
	csp_queue_dequeue_isr(csp_buffers, &buffer, &task_woken);
	if (buffer == NULL)
		return NULL;

	if (buffer != buffer->skbf_addr)
		return NULL;

	buffer->refcount = 1;
	return buffer->skbf_data;

}

void *csp_buffer_get(size_t _data_size) {

	if (_data_size > csp_conf.buffer_data_size) {
		csp_log_error("GET: Attempt to allocate too large data size %u > max %u", (unsigned int) _data_size, (unsigned int) csp_conf.buffer_data_size);
		return NULL;
	}

	csp_skbf_t * buffer = NULL;
	csp_queue_dequeue(csp_buffers, &buffer, 0);
	if (buffer == NULL) {
		csp_log_error("GET: Out of buffers");
		return NULL;
	}

	if (buffer != buffer->skbf_addr) {
		csp_log_error("GET: Corrupt CSP buffer %p != %p", buffer, buffer->skbf_addr);
		return NULL;
	}

	csp_log_buffer("GET: %p", buffer);

	buffer->refcount = 1;
	return buffer->skbf_data;
}

void csp_buffer_free_isr(void *packet) {

	if (packet == NULL) {
		// freeing a NULL pointer is OK, e.g. standard free()
		return;
	}

	csp_skbf_t * buf = (void*)(((uint8_t*)packet) - sizeof(csp_skbf_t));

	if (((uintptr_t) buf % CSP_BUFFER_ALIGN) > 0) {
		return;
	}

	if (buf->skbf_addr != buf) {
		return;
	}

	if (buf->refcount == 0) {
		return;
	}

	if (--(buf->refcount) > 0) {
		return;
	}

	CSP_BASE_TYPE task_woken = 0;
	csp_queue_enqueue_isr(csp_buffers, &buf, &task_woken);

}

void csp_buffer_free(void *packet) {

	if (packet == NULL) {
		/* freeing a NULL pointer is OK, e.g. standard free() */
		return;
	}

	csp_skbf_t * buf = (void*)(((uint8_t*)packet) - sizeof(csp_skbf_t));

	if (((uintptr_t) buf % CSP_BUFFER_ALIGN) > 0) {
		csp_log_error("FREE: Unaligned CSP buffer pointer %p", packet);
		return;
	}

	if (buf->skbf_addr != buf) {
		csp_log_error("FREE: Invalid CSP buffer pointer %p", packet);
		return;
	}

	if (buf->refcount == 0) {
		csp_log_error("FREE: Buffer already free %p", buf);
		return;
	}

	if (--(buf->refcount) > 0) {
		csp_log_error("FREE: Buffer %p in use by %u users", buf, buf->refcount);
		return;
	}

	csp_log_buffer("FREE: %p", buf);
	csp_queue_enqueue(csp_buffers, &buf, 0);

}

void *csp_buffer_clone(void *buffer) {

	csp_packet_t *packet = (csp_packet_t *) buffer;
	if (!packet) {
		return NULL;
	}

	csp_packet_t *clone = csp_buffer_get(packet->length);
	if (clone) {
		memcpy(clone, packet, csp_buffer_size());
	}

	return clone;

}

int csp_buffer_remaining(void) {
	return csp_queue_size(csp_buffers);
}

size_t csp_buffer_size(void) {
	return (csp_conf.buffer_data_size + CSP_BUFFER_PACKET_OVERHEAD);
}

size_t csp_buffer_data_size(void) {
	return csp_conf.buffer_data_size;
}
