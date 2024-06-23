#include "csp_qfifo.h"

#include <csp/arch/csp_queue.h>
#include <csp/csp_debug.h>
#include <csp/csp_buffer.h>
#include "csp_macro.h"
#include "csp/autoconfig.h"

static csp_static_queue_t qfifo_queue __noinit;
static csp_queue_handle_t qfifo_queue_handle __noinit;
static char qfifo_queue_buffer[sizeof(csp_qfifo_t) * CSP_QFIFO_LEN] __noinit;

void csp_qfifo_init(void) {
	qfifo_queue_handle = csp_queue_create_static(CSP_QFIFO_LEN, sizeof(csp_qfifo_t), qfifo_queue_buffer, &qfifo_queue);
}

int csp_qfifo_read(csp_qfifo_t * input) {

	if (csp_queue_dequeue(qfifo_queue_handle, input, FIFO_TIMEOUT) != CSP_QUEUE_OK)
		return CSP_ERR_TIMEDOUT;

	return CSP_ERR_NONE;
}

void csp_qfifo_write(csp_packet_t * packet, csp_iface_t * iface, void * pxTaskWoken) {

	int result;

	if (packet == NULL) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_POINTER;
		return;
	}

	if (iface == NULL) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_POINTER;
		if (pxTaskWoken == NULL)
			csp_buffer_free(packet);
		else
			csp_buffer_free_isr(packet);
		return;
	}

	csp_qfifo_t queue_element;
	queue_element.iface = iface;
	queue_element.packet = packet;

	if (pxTaskWoken == NULL)
		result = csp_queue_enqueue(qfifo_queue_handle, &queue_element, 1);
	else
		result = csp_queue_enqueue_isr(qfifo_queue_handle, &queue_element, pxTaskWoken);

	if (result != CSP_QUEUE_OK) {
		csp_dbg_conn_ovf++;
		iface->tx_error++;
		if (pxTaskWoken == NULL)
			csp_buffer_free(packet);
		else
			csp_buffer_free_isr(packet);
	}
}

void csp_qfifo_wake_up(void) {
	const csp_qfifo_t queue_element = {.iface = NULL, .packet = NULL};
	csp_queue_enqueue(qfifo_queue_handle, &queue_element, 0);
}
