/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_thread.h>
#include <csp/csp_queue.h>
#include <csp/csp_semaphore.h>

/* Static allocation of ports */
csp_port_t ports[17];

void csp_port_init(void) {

	memset(ports, PORT_CLOSED, sizeof(csp_port_t) * 17);

}

/*xQueueHandle csp_port_listener(int conn_queue_length) {

	return xQueueCreate(conn_queue_length, sizeof(conn_t *));

}*/

int csp_listen(csp_socket_t * socket, size_t conn_queue_length) {
    
    if (socket == NULL)
        return -1;

    socket->conn_queue = csp_queue_create(conn_queue_length, sizeof(csp_conn_t *));
    if (socket->conn_queue != NULL) {
        return 0;
    } else {
        return -1;
    }

}

int csp_bind(csp_socket_t * socket, uint8_t port) {
    
	if (port > 16) {
		printf("Only ports from 0-15 (and 16) are available for incoming ports\r\n");
		return -1;
	}

	CSP_ENTER_CRITICAL();

	/* Check if port number is valid */
	if (ports[port].state != PORT_CLOSED) {
		printf("ERROR: Port %d is already in use\r\n", port);
		return -1;
	}

	csp_debug("Binding socket %p to port %u\r\n", socket, port);

	/* Save listener */
	ports[port].callback = NULL;
	ports[port].socket = socket;
	ports[port].state = PORT_OPEN;

	CSP_EXIT_CRITICAL();

    return 0;

}

int csp_bind_callback(void (*callback) (csp_conn_t*), uint8_t port) {

	if (port > 16) {
		printf("Only ports from 0-15 (and 16) are available for incoming ports\r\n");
		return -1;
	}

	CSP_ENTER_CRITICAL();

	/* Check if port number is valid */
	if (ports[port].state != PORT_CLOSED) {
		printf("ERROR: Port %d is already in use\r\n", port);
		return -1;
	}

	/* Save callback */
	ports[port].callback = callback;
	ports[port].socket = NULL;
	ports[port].state = PORT_OPEN;

	CSP_EXIT_CRITICAL();

    return 0;

}


