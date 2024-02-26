#include <csp/csp_buffer.h>

#include <string.h>

#include <csp/arch/csp_queue.h>
#include <csp/csp_debug.h>
#include "csp_macro.h"

//#undef NDEBUG
#include <assert.h>

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

static void csp_buffer_clear(csp_packet_t * packet) {

	packet->data_end = packet->data;
	packet->frame_begin = packet->data;
	packet->length = 0;
	packet->frame_length = 0;
}

csp_packet_t * csp_buffer_get_isr(size_t unused) {

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
	csp_buffer_clear(&buffer->skbf_data);

	return &buffer->skbf_data;
}

csp_packet_t * csp_buffer_get(size_t unused) {

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
	csp_buffer_clear(&buffer->skbf_data);

	return &buffer->skbf_data;
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

void * csp_buffer_clone(void * buffer) {

	csp_packet_t * packet = (csp_packet_t *)buffer;
	if (!packet) {
		return NULL;
	}

	csp_packet_t * clone = csp_buffer_get(0);
	if (clone) {
		size_t size = sizeof(csp_packet_t) - CSP_BUFFER_SIZE + CSP_PACKET_PADDING_BYTES + packet->length;
		memcpy(clone, packet, size > sizeof(csp_packet_t) ? sizeof(csp_packet_t) : size);
	}

	return clone;
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

void check_data_integrity(csp_packet_t * packet) {

	ptrdiff_t header_length;

	header_length = packet->data - packet->frame_begin;
	assert(packet->frame_length == packet->length + header_length);
	assert(packet->data_end - packet->data == packet->length);
}

uint16_t csp_buffer_get_frame_length(csp_packet_t * packet) {

	ptrdiff_t header_length;

	check_data_integrity(packet);

	header_length = packet->data - packet->frame_begin;

	return (packet->data_end - packet->data) + header_length;
}

uint16_t csp_buffer_get_data_length(csp_packet_t * packet) {

	check_data_integrity(packet);

	return packet->data_end - packet->data;
}

bool csp_buffer_has_space(csp_packet_t * packet, size_t len) {

	return len + csp_buffer_get_data_length(packet) <= CSP_BUFFER_SIZE;
}

static bool csp_buffer_has_frame_space(csp_packet_t * packet, size_t len) {

	size_t hdr_len;

	hdr_len = (size_t)(packet->data - packet->frame_begin);

	return len <= CSP_BUFFER_SIZE + hdr_len;
}

static void * csp_buffer_frame_aquire(csp_packet_t * packet, size_t len) {

	void * ret = NULL;

	if (csp_buffer_has_frame_space(packet, len)) {
		ptrdiff_t hdr_len;
		size_t data_len;

		hdr_len = packet->data - packet->frame_begin;
		data_len = len - hdr_len;

		packet->data_end = packet->data + data_len;
		packet->length = data_len;
		packet->frame_length = len;

		ret = packet->frame_begin;
	}

	check_data_integrity(packet);

	return ret;
}

int csp_buffer_frame_replace(csp_packet_t * packet, const void * data, size_t len) {

	void * dst;
	int ret = CSP_ERR_NOMEM;

	dst = csp_buffer_frame_aquire(packet, len);
	if (dst != NULL) {
		memcpy(dst, data, len);
		ret = csp_buffer_get_frame_length(packet);
	}

	return ret;
}

void * csp_buffer_data_aquire(csp_packet_t * packet, size_t len) {

	void * ret = NULL;

	if (csp_buffer_has_space(packet, len)) {
		ret = packet->data_end;
		packet->data_end += len;
		packet->length += len;
		packet->frame_length += len;
	}

	check_data_integrity(packet);

	return ret;
}

int csp_buffer_data_append(csp_packet_t * packet, const void * data, size_t len) {

	void * dst;
	int ret = CSP_ERR_NOMEM;

	dst = csp_buffer_data_aquire(packet, len);
	if (dst != NULL) {
		memcpy(dst, data, len);
		ret = csp_buffer_get_data_length(packet);
	}

	return ret;
}

int csp_buffer_data_append_byte(csp_packet_t * packet, const uint8_t data) {

	return csp_buffer_data_append(packet, &data, sizeof(data));
}

int csp_buffer_data_append_uint32(csp_packet_t * packet, const uint32_t data) {

	return csp_buffer_data_append(packet, &data, sizeof(data));
}

int csp_buffer_data_copy(csp_packet_t * packet, void * dst, size_t len) {

	int ret = CSP_ERR_NOMEM;

	check_data_integrity(packet);

	if (csp_buffer_get_data_length(packet) <= len) {
		memcpy(dst, packet->data, packet->length);
		ret = csp_buffer_get_data_length(packet);
	}

	return ret;
}

static void __csp_buffer_set_header_length(csp_packet_t * packet, size_t len) {

	packet->frame_begin = packet->data - len;
	packet->frame_length = packet->data_end - packet->frame_begin;
}

void csp_buffer_set_header_length(csp_packet_t * packet, size_t len) {

	if (len <= CSP_PACKET_PADDING_BYTES) {
		__csp_buffer_set_header_length(packet, len);
	}
}

static void __csp_buffer_set_data_length(csp_packet_t * packet, size_t len) {

	packet->data_end = packet->data + len;
	packet->length = len;
	packet->frame_length = len + packet->data - packet->frame_begin;
}


void csp_buffer_set_data_length(csp_packet_t * packet, size_t len) {

	if (len <= CSP_BUFFER_SIZE) {
		__csp_buffer_set_data_length(packet, len);
	}

	check_data_integrity(packet);
}

void csp_buffer_data_clear(csp_packet_t * packet) {

	__csp_buffer_set_data_length(packet, 0);

	check_data_integrity(packet);
}

int csp_buffer_data_replace(csp_packet_t * packet, const void * data, size_t len) {

	csp_buffer_data_clear(packet);

	return csp_buffer_data_append(packet, data, len);
}

void csp_buffer_header_clear(csp_packet_t * packet) {

	__csp_buffer_set_header_length(packet, 0);
}
