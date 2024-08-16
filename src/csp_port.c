

#include "csp_port.h"

#include <stdlib.h>
#include <string.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/arch/csp_queue.h>
#include "csp/autoconfig.h"

#include "csp_conn.h"

typedef enum {
	PORT_CLOSED = 0,
	PORT_OPEN = 1,
	PORT_OPEN_CB = 2,
} csp_port_state_t;

typedef struct {
	csp_port_state_t state;
	union {
		csp_socket_t * socket;
		csp_callback_t callback;
	};
} csp_port_t;

/* We rely on the .bss section to clear this, so there is no csp_port_init() function */
static csp_port_t ports[CSP_PORT_MAX_BIND + 2] = {0};

csp_callback_t csp_port_get_callback(unsigned int port) {

	if (port > CSP_PORT_MAX_BIND) {
		return NULL;
	}

	/* Check if port is open callback */
	if (ports[port].state == PORT_OPEN_CB) {
		return ports[port].callback;
	}

	/* If it's open socket, then return no callback */
	if (ports[port].state == PORT_OPEN) {
		return NULL;
	}

	/* Otherwise check if we have a match all callback */
	if (ports[CSP_PORT_MAX_BIND + 1].state == PORT_OPEN_CB) {
		return ports[CSP_PORT_MAX_BIND + 1].callback;
	}

	return NULL;
}

csp_socket_t * csp_port_get_socket(unsigned int port) {

	if (port > CSP_PORT_MAX_BIND) {
		return NULL;
	}

	if (ports[port].state == PORT_OPEN) {
		return ports[port].socket;
	}

	if (ports[port].state == PORT_OPEN_CB) {
		return NULL;
	}

	if (ports[CSP_PORT_MAX_BIND + 1].state == PORT_OPEN) {
		return ports[CSP_PORT_MAX_BIND + 1].socket;
	}

	return NULL;
}

int csp_listen(csp_socket_t * socket, size_t backlog) {
	socket->rx_queue = csp_queue_create_static(CSP_CONN_RXQUEUE_LEN, sizeof(csp_packet_t *), socket->rx_queue_static_data, &socket->rx_queue_static);
	return CSP_ERR_NONE;
}

int csp_bind(csp_socket_t * socket, uint8_t port) {

	if (socket == NULL)
		return CSP_ERR_INVAL;

	if (port == CSP_ANY) {
		port = CSP_PORT_MAX_BIND + 1;
	} else if (port > CSP_PORT_MAX_BIND) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_BIND_PORT;
		return CSP_ERR_INVAL;
	}

	if (ports[port].state != PORT_CLOSED) {
		csp_dbg_errno = CSP_DBG_ERR_PORT_ALREADY_IN_USE;
		return CSP_ERR_USED;
	}

	/* Save listener */
	ports[port].socket = socket;
	ports[port].state = PORT_OPEN;

	return CSP_ERR_NONE;
}

int csp_bind_callback(csp_callback_t callback, uint8_t port) {

	if (callback == NULL) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_POINTER;
		return CSP_ERR_INVAL;
	}

	if (port == CSP_ANY) {
		port = CSP_PORT_MAX_BIND + 1;
	} else if (port > CSP_PORT_MAX_BIND) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_BIND_PORT;
		return CSP_ERR_INVAL;
	}

	if (ports[port].state != PORT_CLOSED) {
		csp_dbg_errno = CSP_DBG_ERR_PORT_ALREADY_IN_USE;
		return CSP_ERR_ALREADY;
	}

	/* Save listener */
	ports[port].callback = callback;
	ports[port].state = PORT_OPEN_CB;

	return CSP_ERR_NONE;

}

int csp_socket_close(csp_socket_t * sock) {
	if (sock == NULL) {
		return CSP_ERR_NONE;
	}

	for (size_t i = 0; i < CSP_PORT_MAX_BIND + 2; i++) {
		csp_port_t * port = &ports[i];

		if (port->state == PORT_OPEN && port->socket == sock) {
			port->state = PORT_CLOSED;
			port->socket = NULL;
			break;
		}
	}

	if (sock->rx_queue != NULL) {
		csp_packet_t * packet = NULL;

		while (csp_queue_dequeue(sock->rx_queue, &packet, 0) == CSP_QUEUE_OK) {
			if (packet != NULL) {
				csp_buffer_free(packet);
			}
		}
		csp_queue_free(sock->rx_queue);
	}

	return CSP_ERR_NONE;
}
