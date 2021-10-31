

#include "csp_transport.h"

#include <csp/csp.h>
#include <csp/arch/csp_queue.h>

#include "../csp_conn.h"

void csp_udp_new_packet(csp_conn_t * conn, csp_packet_t * packet) {

	/* Enqueue */
	if (csp_conn_enqueue_packet(conn, packet) < 0) {
		csp_log_error("Connection buffer queue full!");
		csp_buffer_free(packet);
		return;
	}

	/* Try to queue up the new connection pointer */
	if (conn->dest_socket != NULL) {
		if (csp_queue_enqueue(conn->dest_socket->rx_queue, &conn, 0) != CSP_QUEUE_OK) {
			csp_log_warn("Warning socket connection queue full");
			csp_close(conn);
			return;
		}

		/* Ensure that this connection will not be posted to this socket again */
		conn->dest_socket = NULL;
	}
}
