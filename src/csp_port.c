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
#include <csp/arch/csp_malloc.h>

#include "csp_conn.h"
#include "csp_init.h"

/* Dynamic allocated port array */
static csp_port_t * ports;

csp_socket_t * csp_port_get_socket(unsigned int port) {

	if (port > csp_conf.port_max_bind) {
		return NULL;
	}

	/* Match dport to socket or local "catch all" port number */
	if (ports[port].state == PORT_OPEN) {
		return ports[port].socket;
	}

	if (ports[csp_conf.port_max_bind + 1].state == PORT_OPEN) {
		return ports[csp_conf.port_max_bind + 1].socket;
	}

	return NULL;

}

int csp_port_init(void) {

	ports = csp_calloc(csp_conf.port_max_bind + 2, sizeof(*ports)); // +2 for max port and CSP_ANY
	if (ports == NULL) {
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;

}

void csp_port_free_resources(void) {

	csp_free(ports);
	ports = NULL;
}

int csp_listen(csp_socket_t * socket, size_t backlog) {
	
	if (socket == NULL)
		return CSP_ERR_INVAL;

	socket->socket = csp_queue_create(backlog, sizeof(csp_conn_t *));
	if (socket->socket == NULL)
		return CSP_ERR_NOMEM;

        socket->opts |= CSP_SO_INTERNAL_LISTEN;

	return CSP_ERR_NONE;

}

int csp_bind(csp_socket_t * socket, uint8_t port) {
	
	if (socket == NULL)
		return CSP_ERR_INVAL;

	if (port == CSP_ANY) {
		port = csp_conf.port_max_bind + 1;
	} else if (port > csp_conf.port_max_bind) {
		csp_log_error("csp_bind: invalid port %u, only ports from 0-%u (+ CSP_ANY for default) are available for incoming ports", port, csp_conf.port_max_bind);
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


