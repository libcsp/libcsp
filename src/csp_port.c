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

#include "csp_port.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <csp/csp.h>
#include <csp/arch/csp_queue.h>
#include <csp_autoconfig.h>

#include "csp_conn.h"

typedef enum {
	PORT_CLOSED = 0,
	PORT_OPEN = 1,
} csp_port_state_t;

typedef struct {
	csp_port_state_t state;
	csp_socket_t * socket;		  // New connections are added to this socket's conn queue
} csp_port_t;

/* We rely on the .bss section to clear this, so there is no csp_port_init() function */
static csp_port_t ports[CSP_PORT_MAX_BIND + 2] = {0};

csp_socket_t * csp_port_get_socket(unsigned int port) {

	if (port > CSP_PORT_MAX_BIND) {
		return NULL;
	}

	/* Match dport to socket or local "catch all" port number */
	if (ports[port].state == PORT_OPEN) {
		return ports[port].socket;
	}

	if (ports[CSP_PORT_MAX_BIND + 1].state == PORT_OPEN) {
		return ports[CSP_PORT_MAX_BIND + 1].socket;
	}

	return NULL;

}

int csp_listen(csp_socket_t * socket, size_t backlog) {
	return CSP_ERR_NONE;
}

int csp_bind(csp_socket_t * socket, uint8_t port) {
	
	if (socket == NULL)
		return CSP_ERR_INVAL;

	if (port == CSP_ANY) {
		port = CSP_PORT_MAX_BIND + 1;
	} else if (port > CSP_PORT_MAX_BIND) {
		csp_log_error("csp_bind: invalid port %u", port);
		return CSP_ERR_INVAL;
	}

	if (ports[port].state != PORT_CLOSED) {
		csp_log_error("Port %d is already in use", port);
		return CSP_ERR_USED;
	}

	csp_log_info("Binding socket %p to port %u", socket, port);

	/* Save listener */
	ports[port].socket = socket;
	ports[port].state = PORT_OPEN;

	return CSP_ERR_NONE;

}


