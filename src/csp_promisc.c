#include "csp_promisc.h"

#include <csp/csp.h>
#include "csp_macro.h"
#include <csp/arch/csp_queue.h>

static csp_queue_handle_t csp_promisc_queue = NULL;
static csp_static_queue_t csp_promisc_queue_static __noinit;
static char csp_promisc_queue_buffer[sizeof(csp_packet_t *) * CSP_CONN_RXQUEUE_LEN] __noinit;

static int csp_promisc_enabled = 0;

int csp_promisc_enable(unsigned int queue_size) {

	/* If queue already initialised */
	if (csp_promisc_queue != NULL) {
		csp_promisc_enabled = 1;
		return CSP_ERR_USED;
	}

	/* Create packet queue */
	csp_promisc_queue = csp_queue_create_static(CSP_CONN_RXQUEUE_LEN, sizeof(csp_packet_t *), csp_promisc_queue_buffer, &csp_promisc_queue_static);

	if (csp_promisc_queue == NULL)
		return CSP_ERR_INVAL;

	csp_promisc_enabled = 1;
	return CSP_ERR_NONE;
}

void csp_promisc_disable(void) {
	csp_promisc_enabled = 0;
}

csp_packet_t * csp_promisc_read(uint32_t timeout) {

	if (csp_promisc_queue == NULL)
		return NULL;

	csp_packet_t * packet = NULL;
	csp_queue_dequeue(csp_promisc_queue, &packet, timeout);

	return packet;
}

void csp_promisc_add(csp_packet_t * packet) {

	if (csp_promisc_enabled == 0)
		return;

	if (csp_promisc_queue != NULL) {
		/* Make a copy of the message and queue it to the promiscuous task */
		csp_packet_t * packet_copy = csp_buffer_clone(packet);
		if (packet_copy != NULL) {
			if (csp_queue_enqueue(csp_promisc_queue, &packet_copy, 0) != CSP_QUEUE_OK) {
				csp_dbg_conn_ovf++;
				csp_buffer_free(packet_copy);
			}
		}
	}
}
