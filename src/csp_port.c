
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <csp/csp.h>

//#define DEBUG

/* Static allocation of ports */
port_t ports[17];

void csp_port_init(void) {

	memset(ports, 0, sizeof(port_t) * 17);

}

xQueueHandle csp_port_listener(int conn_queue_length) {

	return xQueueCreate(conn_queue_length, sizeof(conn_t *));

}

void csp_port_bind(unsigned char port_nr, xQueueHandle listener) {

	if (port_nr > 16) {
		printf("Only ports from 0-15 (and 16) are available for incoming ports\r\n");
		return;
	}

	/* Check if port number is valid */
	if (ports[port_nr].state != PORT_CLOSED) {
		printf("ERROR: Port %d is already in use\r\n", port_nr);
		return;
	}

	//printf("Binding queue %p to port %u\r\n", listener, port_nr);

	/* Save listener */
	ports[port_nr].callback = NULL;
	ports[port_nr].connQueue = listener;
	ports[port_nr].state = PORT_OPEN;

}

void csp_port_callback(unsigned char port_nr, void (*callback) (conn_t*)) {

	if (port_nr > 16) {
		printf("Only ports from 0-15 (and 16) are available for incoming ports\r\n");
		return;
	}

	/* Check if port number is valid */
	if (ports[port_nr].state != PORT_CLOSED) {
		printf("ERROR: Port %d is already in use\r\n", port_nr);
		return;
	}

	/* Save callback */
	ports[port_nr].callback = callback;
	ports[port_nr].connQueue = NULL;
	ports[port_nr].state = PORT_OPEN;

}


