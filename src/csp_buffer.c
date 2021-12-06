#include <csp/csp_buffer.h>

#include <string.h>

#include <csp/arch/csp_queue.h>
#include <csp/csp_debug.h>

#ifndef CSP_BUFFER_ALIGN
#define CSP_BUFFER_ALIGN (sizeof(int *))
#endif

/** Internal buffer header */
typedef struct csp_skbf_s {
	unsigned int refcount;
	void * skbf_addr;
	char skbf_data[];  // -> csp_packet_t
} csp_skbf_t;

#define SKBUF_SIZE CSP_BUFFER_ALIGN *((sizeof(csp_skbf_t) + CSP_BUFFER_SIZE + CSP_BUFFER_PACKET_OVERHEAD + (CSP_BUFFER_ALIGN - 1)) / CSP_BUFFER_ALIGN)

// Queue of free CSP buffers
static csp_queue_handle_t csp_buffers;

void csp_buffer_init(void) {

	/**
	 * Chunk of memory allocated for CSP buffers:
	 * This is marked as .noinit, because csp buffers can never be assumed zeroed out
	 * Putting this section in a separate non .bss area, saves some boot time */
	static char csp_buffer_pool[SKBUF_SIZE * CSP_BUFFER_COUNT] __attribute__((section(".noinit")));
	static csp_static_queue_t csp_buffers_queue __attribute__((section(".noinit")));
	static char csp_buffer_queue_data[CSP_BUFFER_COUNT * sizeof(csp_skbf_t *)] __attribute__((section(".noinit")));

	csp_buffers = csp_queue_create_static(CSP_BUFFER_COUNT, sizeof(csp_skbf_t *), csp_buffer_queue_data, &csp_buffers_queue);

	for (unsigned int i = 0; i < CSP_BUFFER_COUNT; i++) {
		csp_skbf_t * buf = (void *)&csp_buffer_pool[i * SKBUF_SIZE];
		buf->skbf_addr = buf;
		csp_queue_enqueue(csp_buffers, &buf, 0);
	}
}

void * csp_buffer_get_isr(size_t _data_size) {

	if (_data_size > CSP_BUFFER_SIZE)
		return NULL;

	csp_skbf_t * buffer = NULL;
	int task_woken = 0;
	csp_queue_dequeue_isr(csp_buffers, &buffer, &task_woken);
	if (buffer == NULL) {
		csp_dbg_buffer_out++;
		return NULL;
	}

	if (buffer != buffer->skbf_addr) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		/* Best option here must be to leak the invalid buffer */
		return NULL;
	}

	buffer->refcount = 1;
	return buffer->skbf_data;
}

void * csp_buffer_get(size_t _data_size) {

	if (_data_size > CSP_BUFFER_SIZE) {
		csp_dbg_errno = CSP_DBG_ERR_MTU_EXCEEDED;
		return NULL;
	}

	csp_skbf_t * buffer = NULL;
	csp_queue_dequeue(csp_buffers, &buffer, 0);
	if (buffer == NULL) {
		csp_dbg_buffer_out++;
		return NULL;
	}

	if (buffer != buffer->skbf_addr) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		return NULL;
	}

	buffer->refcount = 1;
	return buffer->skbf_data;
}

void csp_buffer_free_isr(void * packet) {

	if (packet == NULL) {
		// freeing a NULL pointer is OK, e.g. standard free()
		return;
	}

	csp_skbf_t * buf = (void *)(((uint8_t *)packet) - sizeof(csp_skbf_t));

	if (((uintptr_t)buf % CSP_BUFFER_ALIGN) > 0) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		return;
	}

	if (buf->skbf_addr != buf) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		return;
	}

	if (buf->refcount == 0) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		return;
	}

	if (--(buf->refcount) > 0) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;;
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

	csp_skbf_t * buf = (void *)(((uint8_t *)packet) - sizeof(csp_skbf_t));

	if (((uintptr_t)buf % CSP_BUFFER_ALIGN) > 0) {
		csp_dbg_errno = CSP_DBG_ERR_CORRUPT_BUFFER;
		return;
	}

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

void * csp_buffer_clone(void * buffer) {

	csp_packet_t * packet = (csp_packet_t *)buffer;
	if (!packet) {
		return NULL;
	}

	csp_packet_t * clone = csp_buffer_get(packet->length);
	if (clone) {
		memcpy(clone, packet, csp_buffer_size());
	}

	return clone;
}

int csp_buffer_remaining(void) {
	return csp_queue_size(csp_buffers);
}

size_t csp_buffer_size(void) {
	return (CSP_BUFFER_SIZE + CSP_BUFFER_PACKET_OVERHEAD);
}

size_t csp_buffer_data_size(void) {
	return CSP_BUFFER_SIZE;
}
