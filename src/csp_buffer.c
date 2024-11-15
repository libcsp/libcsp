#include <csp/csp_buffer.h>

#include <string.h>

#include <csp/arch/csp_queue.h>
#include <csp/csp_debug.h>
#include "csp_macro.h"
#include <csp/csp_hooks.h>
#include <csp/csp_id.h>

/** Internal buffer header */
typedef struct csp_skbf_s {
	unsigned int refcount;
	void * skbf_addr;
	csp_packet_t skbf_data;
} csp_skbf_t;

// Queue of free CSP buffers
static csp_queue_handle_t csp_buffers;

void csp_buffer_init(void) {
	/**
	 * Chunk of memory allocated for CSP buffers:
	 * This is marked as .noinit, because csp buffers can never be assumed zeroed out
	 * Putting this section in a separate non .bss area, saves some boot time */
	static csp_skbf_t csp_buffer_pool[CSP_BUFFER_COUNT]  __noinit;
	static csp_static_queue_t csp_buffers_queue __noinit;
	static char csp_buffer_queue_data[CSP_BUFFER_COUNT * sizeof(csp_skbf_t *)] __noinit;

	csp_buffers = csp_queue_create_static(CSP_BUFFER_COUNT, sizeof(csp_skbf_t *), csp_buffer_queue_data, &csp_buffers_queue);

	for (unsigned int i = 0; i < CSP_BUFFER_COUNT; i++) {
		csp_buffer_pool[i].skbf_addr = &csp_buffer_pool[i];
		csp_skbf_t * bufptr = &csp_buffer_pool[i];
		csp_queue_enqueue(csp_buffers, &bufptr, 0);
	}
}

static csp_packet_t * csp_packet_init(csp_packet_t * packet)
{

#if (CSP_BUFFER_ZERO_CLEAR)
	memset(packet, 0, sizeof(csp_packet_t));
#endif

	packet->length = 0;
	packet->frame_begin = packet->data;
	packet->frame_length = 0;

	csp_id_clear(&packet->id);

	return packet;
}

static csp_packet_t * csp_buffer_get_actual(int reserve, int isr) {

	/* Get buffers remaining */
	int remain;
	if (isr) {
		remain = csp_queue_size_isr(csp_buffers);
	} else {
		remain = csp_queue_size(csp_buffers);
	}
	/* Respect if remaining is lower than the reserve requested */
	if (remain < reserve) {
		return NULL;
	}

	/* Now fetch a buffer */
	csp_skbf_t * buf = NULL;
	if (isr) {
		int task_woken = 0;
		csp_queue_dequeue_isr(csp_buffers, &buf, &task_woken);
	} else {
		csp_queue_dequeue(csp_buffers, &buf, 0);
	}

	/* We might be out of buffers */
	if (buf == NULL) {
		csp_dbg_buffer_out++;
		return NULL;
	}

	if (buf != buf->skbf_addr) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		return NULL;
	}

	buf->refcount = 1;

	return csp_packet_init(&buf->skbf_data);
}

void csp_buffer_free_isr(void * packet) {

	if (packet == NULL) {
		// freeing a NULL pointer is OK, e.g. standard free()
		return;
	}

	csp_skbf_t * buf = CONTAINER_OF(packet, csp_skbf_t, skbf_data);

	if (buf->skbf_addr != buf) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		return;
	}

	if (buf->refcount == 0) {
		csp_dbg_errno = CSP_DBG_ERR_ALREADY_FREE;
		return;
	}

	if (--(buf->refcount) > 0) {
		csp_dbg_errno = CSP_DBG_ERR_REFCOUNT;
		return;
	}

	int task_woken = 0;
	csp_queue_enqueue_isr(csp_buffers, &buf, &task_woken);
}

void csp_buffer_free(void * packet) {

	if (packet == NULL) {
		/* freeing a NULL pointer is OK, e.g. standard free() */
		return;
	}

	csp_skbf_t * buf = CONTAINER_OF(packet, csp_skbf_t, skbf_data);

	if (buf->skbf_addr != buf) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		return;
	}

	if (buf->refcount == 0) {
		csp_dbg_errno = CSP_DBG_ERR_ALREADY_FREE;
		return;
	}

	if (--(buf->refcount) > 0) {
		csp_dbg_errno = CSP_DBG_ERR_REFCOUNT;
		return;
	}

	csp_queue_enqueue(csp_buffers, &buf, 0);
}

csp_packet_t * csp_buffer_clone(const csp_packet_t * packet) {
	csp_packet_t * clone = NULL;
	if (packet) {
		clone = csp_buffer_get(0);
		csp_buffer_copy(packet, clone);
	}

	return clone;
}

void csp_buffer_copy(const csp_packet_t * src, csp_packet_t * dst) {
	if ((NULL != src) && (NULL != dst)) {
		(void)memcpy(dst, src, sizeof(csp_packet_t));
    }
}

void csp_buffer_refc_inc(void * buffer) {

	if (!buffer) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_POINTER;
		return;
	}

	csp_skbf_t * buf = CONTAINER_OF(buffer, csp_skbf_t, skbf_data);
	if (buf->skbf_addr != buf) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		return;
	}
    buf->refcount++;

}

int csp_buffer_remaining(void) {
	return csp_queue_size(csp_buffers);
}

/* CSP will use every remaining buffer in an attempt to allocate a packet
 * buffer. Including the last two, that is normally reserved.
 * This can be important for ensuring a node is always reachable, so a
 * failure to allocate will cause a panic and a reboot */

csp_packet_t * csp_buffer_get_always(void) {
	csp_packet_t * packet = csp_buffer_get_actual(0, 0);
	if (packet == NULL) {
		csp_panic("Out of buffers");
		while(1);
	}
	return packet;
}

csp_packet_t * csp_buffer_get_always_isr(void) {
	csp_packet_t * packet = csp_buffer_get_actual(0, 1);
	if (packet == NULL) {
		csp_panic("Out of buffers");
		while(1);
	}
	return packet;
}

/* CSP will try to reserve the last two buffers for calls which can take it,
 * examples are client funktions that are allowed to fail and have adequate
 * error checking. Or services which are allowed to timeout of memory becomes
 * sparse. */

csp_packet_t * csp_buffer_get(size_t unused) {
	return csp_buffer_get_actual(2, 0);
}

csp_packet_t * csp_buffer_get_isr(size_t unused) {
	return csp_buffer_get_actual(2, 1);
}
