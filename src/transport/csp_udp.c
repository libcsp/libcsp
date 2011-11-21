/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk)

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

#include <csp/csp.h>
#include "../arch/csp_queue.h"
#include "../csp_port.h"
#include "../csp_conn.h"

void csp_udp_new_packet(csp_conn_t * conn, csp_packet_t * packet) {

	/* Enqueue */
	if (csp_conn_enqueue_packet(conn, packet) < 0) {
		csp_debug(CSP_ERROR, "Connection buffer queue full!\r\n");
		csp_buffer_free(packet);
		return;
	}

	/* Try to queue up the new connection pointer */
	if (conn->socket != NULL) {
		if (csp_queue_enqueue(conn->socket, &conn, 0) != CSP_QUEUE_OK) {
			csp_debug(CSP_WARN, "Warning Routing Queue Full\r\n");
			csp_close(conn);
			return;
		}

		/* Ensure that this connection will not be posted to this socket again */
		conn->socket = NULL;
	}

}

