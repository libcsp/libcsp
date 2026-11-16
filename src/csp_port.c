

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
	PORT_CLOSING = 3,
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

static pthread_mutex_t ports_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ports_cond = PTHREAD_COND_INITIALIZER;

static unsigned int active_callbacks[CSP_PORT_MAX_BIND + 2] = {0};

/*
 * Dispatch callback associated with a destination port.
 *
 * Returns true if a callback was dispatched.
 * Returns false if no callback is available or port is used for socket.
 *
 */
bool csp_port_dispatch_callback(csp_packet_t *packet,
                                unsigned int port,
                                uint32_t security_opts,
                                csp_iface_t *iface,
                                csp_security_check_cb_t validate) {
    if (packet == NULL || port > CSP_PORT_MAX_BIND) {
        return false;
    }

    unsigned int callback_port = port;
    csp_callback_t callback = NULL;

    pthread_mutex_lock(&ports_mutex);

    if (ports[port].state == PORT_OPEN_CB) {

        callback = ports[port].callback;

    } else if (ports[port].state == PORT_OPEN ||
               ports[port].state == PORT_CLOSING) {

        /* Do not dispatch to CSP_ANY if a specific port is occupied or closing. */
        pthread_mutex_unlock(&ports_mutex);
        return false;

    } else {

        if (ports[CSP_PORT_MAX_BIND + 1].state == PORT_OPEN_CB) {
            callback_port = CSP_PORT_MAX_BIND + 1;
            callback = ports[CSP_PORT_MAX_BIND + 1].callback;
        }
    }

    if (callback == NULL) {
        pthread_mutex_unlock(&ports_mutex);
        return false;
    }

    active_callbacks[callback_port]++;

    pthread_mutex_unlock(&ports_mutex);

    /* Validate packet before invoking the callback. */
    if (validate != NULL &&
        validate(security_opts, iface, packet) != CSP_ERR_NONE) {
        csp_buffer_free(packet);
    } else {
        callback(packet);
    }

    pthread_mutex_lock(&ports_mutex);

    active_callbacks[callback_port]--;

    if (active_callbacks[callback_port] == 0) {
        pthread_cond_broadcast(&ports_cond);
    }

    pthread_mutex_unlock(&ports_mutex);

    return true;
}

csp_socket_t * csp_port_get_socket(unsigned int port) {

	if (port > CSP_PORT_MAX_BIND) {
		return NULL;
	}

	csp_socket_t * socket = NULL;

	pthread_mutex_lock(&ports_mutex);

	if (ports[port].state == PORT_OPEN) {
		socket = ports[port].socket;

	} else if (ports[port].state == PORT_OPEN_CB ||
			   ports[port].state == PORT_CLOSING) {
		socket = NULL;

	} else {
		unsigned int any_port = CSP_PORT_MAX_BIND + 1;

		if (ports[any_port].state == PORT_OPEN) {
			socket = ports[any_port].socket;
		}
	}

	pthread_mutex_unlock(&ports_mutex);

	return socket;
}

/* Create the socket rx_queue if not already created.  The queue
 * holds packets for connection-less sockets and pending connections
 * for connection-oriented sockets. */
static void csp_socket_init_rx_queue(csp_socket_t * socket) {
	if (socket->rx_queue == NULL) {
		socket->rx_queue = csp_queue_create_static(CSP_CONN_RXQUEUE_LEN, sizeof(csp_packet_t *), socket->rx_queue_static_data, &socket->rx_queue_static);
	}
}

bool csp_socket_is_ready_to_receive(const csp_socket_t * socket) {
	return socket->rx_queue != NULL;
}

bool csp_socket_is_conn_less(const csp_socket_t * socket) {
	return (socket->opts & CSP_SO_CONN_LESS) != 0;
}

int csp_listen(csp_socket_t * socket, size_t backlog) {
	(void)backlog; /* Avoid compiler warnings about unused parameter */
	csp_socket_init_rx_queue(socket);
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
	pthread_mutex_lock(&ports_mutex);

	if (ports[port].state != PORT_CLOSED) {
		pthread_mutex_unlock(&ports_mutex);
		csp_dbg_errno = CSP_DBG_ERR_PORT_ALREADY_IN_USE;
		return CSP_ERR_USED;
	}

	if (csp_socket_is_conn_less(socket)) {
    	csp_socket_init_rx_queue(socket);
	}

	/* Save listener */
	ports[port].socket = socket;
	ports[port].state = PORT_OPEN;
	pthread_mutex_unlock(&ports_mutex);

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
	pthread_mutex_lock(&ports_mutex);

	if (ports[port].state != PORT_CLOSED) {
		pthread_mutex_unlock(&ports_mutex);
		csp_dbg_errno = CSP_DBG_ERR_PORT_ALREADY_IN_USE;
		return CSP_ERR_ALREADY;
	}

	/* Save listener */
	ports[port].callback = callback;
	ports[port].state = PORT_OPEN_CB;
	pthread_mutex_unlock(&ports_mutex);

	return CSP_ERR_NONE;

}

int csp_socket_close(csp_socket_t * sock) {
	if (sock == NULL) {
		return CSP_ERR_NONE;
	}
	/*
	 * Remove socket from port table.
	 * Do not hold ports_mutex while draining/freeing the queue.
	 */
	pthread_mutex_lock(&ports_mutex);

	for (size_t i = 0; i < CSP_PORT_MAX_BIND + 2; i++) {
		csp_port_t * port = &ports[i];

		if (port->state == PORT_OPEN && port->socket == sock) {
			port->state = PORT_CLOSED;
			port->socket = NULL;
			break;
		}
	}
	pthread_mutex_unlock(&ports_mutex);

	if (sock->rx_queue != NULL) {
		csp_packet_t * packet = NULL;

		while (csp_queue_dequeue(sock->rx_queue, &packet, 0) == CSP_QUEUE_OK) {
			if (packet != NULL) {
				csp_buffer_free(packet);
			}
		}
		csp_queue_free(sock->rx_queue);
		sock->rx_queue = NULL;
	}

	return CSP_ERR_NONE;
}

int csp_unbind_callback(uint8_t port) {

	if (port == CSP_ANY) {
		port = CSP_PORT_MAX_BIND + 1;
	} else if (port > CSP_PORT_MAX_BIND) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_BIND_PORT;
		return CSP_ERR_INVAL;
	}
	pthread_mutex_lock(&ports_mutex);

	if (ports[port].state == PORT_OPEN) {
		/* PORT_OPEN is used for socket-bound ports. Use csp_socket_close() to free the socket resources. */
		pthread_mutex_unlock(&ports_mutex);
		csp_dbg_errno = CSP_DBG_ERR_INVALID_BIND_PORT;
		return CSP_ERR_INVAL;
	}

	if (ports[port].state == PORT_CLOSED) {
		pthread_mutex_unlock(&ports_mutex);
		return CSP_ERR_NONE;
	}
	if (ports[port].state == PORT_CLOSING) {
		/*
		 * Another thread is already unbinding this port.
		 * Wait until it completes.
		 */
		while (ports[port].state == PORT_CLOSING) {
			pthread_cond_wait(&ports_cond, &ports_mutex);
		}
		pthread_mutex_unlock(&ports_mutex);
		return CSP_ERR_NONE;
	}

	ports[port].state = PORT_CLOSING;

	while (active_callbacks[port] > 0) {
		pthread_cond_wait(&ports_cond, &ports_mutex);
	}

	ports[port].state = PORT_CLOSED;
	ports[port].callback = NULL;
	pthread_cond_broadcast(&ports_cond);
	pthread_mutex_unlock(&ports_mutex);

	return CSP_ERR_NONE;
}
