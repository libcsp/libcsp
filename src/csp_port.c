#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/thread.h>
#include <csp/queue.h>
#include <csp/semaphore.h>

/* Static allocation of ports */
port_t ports[17];

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

// @todo Lock port array
int csp_bind(csp_socket_t * socket, uint8_t port) {
    
	if (port > 16) {
		printf("Only ports from 0-15 (and 16) are available for incoming ports\r\n");
		return -1;
	}

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

    return 0;

}

int csp_bind_callback(void (*callback) (conn_t*), uint8_t port) {

	if (port > 16) {
		printf("Only ports from 0-15 (and 16) are available for incoming ports\r\n");
		return -1;
	}

	/* Check if port number is valid */
	if (ports[port].state != PORT_CLOSED) {
		printf("ERROR: Port %d is already in use\r\n", port);
		return -1;
	}

	/* Save callback */
	ports[port_nr].callback = callback;
	ports[port_nr].socket = NULL;
	ports[port_nr].state = PORT_OPEN;

    return 0;

}


