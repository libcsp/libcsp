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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>

#include "arch/csp_thread.h"
#include "arch/csp_queue.h"
#include "arch/csp_semaphore.h"

#include "csp_port.h"
#include "csp_conn.h"

/* Allocation of ports */
csp_port_t ports[CSP_MAX_BIND_PORT + 2];

static csp_bin_sem_handle_t port_lock;

void csp_port_init(void) {

	memset(ports, PORT_CLOSED, sizeof(csp_port_t) * (CSP_MAX_BIND_PORT + 2));

	if (csp_bin_sem_create(&port_lock) != CSP_SEMAPHORE_OK)
		csp_debug(CSP_ERROR, "No more memory for port semaphore\r\n");

}

int csp_listen(csp_socket_t * socket, size_t conn_queue_length) {
    
    if (socket == NULL)
        return -1;

    socket->queue = csp_queue_create(conn_queue_length, sizeof(csp_conn_t *));
    if (socket->queue != NULL) {
        return 0;
    } else {
        return -1;
    }

}

int csp_bind(csp_socket_t * socket, uint8_t port) {
    
	if (port > CSP_ANY) {
		csp_debug(CSP_ERROR, "Only ports from 0-%u (and CSP_ANY for default) are available for incoming ports\r\n", CSP_ANY);
		return -1;
	}

	if (csp_bin_sem_wait(&port_lock, 100) != CSP_SEMAPHORE_OK) {
		csp_debug(CSP_ERROR, "Failed to lock port array\r\n");
		return -1;
	}

	/* Check if port number is valid */
	if (ports[port].state != PORT_CLOSED) {
		csp_debug(CSP_ERROR, "Port %d is already in use\r\n", port);
		csp_bin_sem_post(&port_lock);
		return -1;
	}

	csp_debug(CSP_INFO, "Binding socket %p to port %u\r\n", socket, port);

	/* Save listener */
	ports[port].socket = socket;
	ports[port].state = PORT_OPEN;

	csp_bin_sem_post(&port_lock);

    return 0;

}


