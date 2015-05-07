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

#ifndef CSP_BUFFER_ALIGN
#define CSP_BUFFER_ALIGN	(sizeof(int *))
#endif

typedef struct csp_skbf_s {
	unsigned int refcount;
	void * skbf_addr;
	char skbf_data[];
} csp_skbf_t;

static csp_queue_handle_t csp_buffers;
static char * csp_buffer_pool;
static unsigned int count, size;

CSP_DEFINE_CRITICAL(csp_critical_lock);

int csp_buffer_init(int buf_count, int buf_size) {

	unsigned int i;
	csp_skbf_t * buf;

	count = buf_count;
	size = buf_size + CSP_BUFFER_PACKET_OVERHEAD;
	unsigned int skbfsize = (sizeof(csp_skbf_t) + size);
	skbfsize = CSP_BUFFER_ALIGN * ((skbfsize + CSP_BUFFER_ALIGN - 1) / CSP_BUFFER_ALIGN);
	unsigned int poolsize = count * skbfsize;

	csp_buffer_pool = csp_malloc(poolsize);
	if (csp_buffer_pool == NULL)
		goto fail_malloc;

	csp_buffers = csp_queue_create(count, sizeof(void *));
	if (!csp_buffers)
		goto fail_queue;

	if (CSP_INIT_CRITICAL(csp_critical_lock) != CSP_ERR_NONE)
		goto fail_critical;

	memset(csp_buffer_pool, 0, poolsize);

	for (i = 0; i < count; i++) {

		/* We have already taken care of pointer alignment since
		 * skbfsize is an integer multiple of sizeof(int *)
		 * but the explicit cast to a void * is still necessary
		 * to tell the compiler so.
		 */
		buf = (void *) &csp_buffer_pool[i * skbfsize];
		buf->refcount = 0;
		buf->skbf_addr = buf;

		csp_queue_enqueue(csp_buffers, &buf, 0);

	}

	return CSP_ERR_NONE;

fail_critical:
	csp_queue_remove(csp_buffers);
fail_queue:
	csp_free(csp_buffer_pool);
fail_malloc:
	return CSP_ERR_NOMEM;

}

void *csp_buffer_get_isr(size_t buf_size) {

	csp_skbf_t * buffer = NULL;
	CSP_BASE_TYPE task_woken = 0;

	if (buf_size + CSP_BUFFER_PACKET_OVERHEAD > size)
		return NULL;

	csp_queue_dequeue_isr(csp_buffers, &buffer, &task_woken);
	if (buffer == NULL)
		return NULL;

	if (buffer != buffer->skbf_addr)
		return NULL;

	buffer->refcount++;
	return buffer->skbf_data;

}

void *csp_buffer_get(size_t buf_size) {

	csp_skbf_t * buffer = NULL;

	if (buf_size + CSP_BUFFER_PACKET_OVERHEAD > size) {
		csp_log_error("Attempt to allocate too large block %u", buf_size);
		return NULL;
	}

	csp_queue_dequeue(csp_buffers, &buffer, 0);
	if (buffer == NULL) {
		csp_log_error("Out of buffers");
		return NULL;
	}

	csp_log_buffer("GET: %p %p", buffer, buffer->skbf_addr);

	if (buffer != buffer->skbf_addr) {
		csp_log_error("Corrupt CSP buffer");
		return NULL;
	}

	buffer->refcount++;
	return buffer->skbf_data;
}

void csp_buffer_free_isr(void *packet) {
	CSP_BASE_TYPE task_woken = 0;
	if (!packet)
		return;

	csp_skbf_t * buf = packet - sizeof(csp_skbf_t);

	if (((uintptr_t) buf % CSP_BUFFER_ALIGN) > 0)
		return;

	if (buf->skbf_addr != buf)
		return;

	if (buf->refcount == 0) {
		return;
	} else if (buf->refcount > 1) {
		buf->refcount--;
		return;
	} else {
		buf->refcount = 0;
		csp_queue_enqueue_isr(csp_buffers, &buf, &task_woken);
	}

}

void csp_buffer_free(void *packet) {
	if (!packet) {
		csp_log_error("Attempt to free null pointer");
		return;
	}

	csp_skbf_t * buf = packet - sizeof(csp_skbf_t);

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
	} else if (buf->refcount > 1) {
		buf->refcount--;
		csp_log_error("FREE: Buffer %p in use by %u users", buf, buf->refcount);
		return;
	} else {
		buf->refcount = 0;
		csp_log_buffer("FREE: %p", buf);
		csp_queue_enqueue(csp_buffers, &buf, 0);
	}

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
