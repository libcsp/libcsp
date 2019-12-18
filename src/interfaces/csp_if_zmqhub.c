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

#include <csp/interfaces/csp_if_zmqhub.h>

#include <assert.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_semaphore.h>

/* ZMQ */
#include <zmq.h>

#define CSP_ZMQ_MTU   1024   // max payload data, see documentation

/* ZMQ driver & interface */
typedef struct {
	csp_thread_handle_t rx_thread;
	void * context;
	void * publisher;
	void * subscriber;
	csp_bin_sem_handle_t tx_wait;
	csp_iface_t interface;
} zmq_driver_t;

/**
 * Interface transmit function
 * @param packet Packet to transmit
 * @param timeout Timeout in ms
 * @return 1 if packet was successfully transmitted, 0 on error
 */
int csp_zmqhub_tx(const csp_rtable_route_t * route, csp_packet_t * packet, uint32_t timeout) {

	zmq_driver_t * drv = route->interface->driver_data;

	const uint8_t dest = (route->mac != CSP_NODE_MAC) ? route->mac : packet->id.dst;

	uint16_t length = packet->length;
	uint8_t * destptr = ((uint8_t *) &packet->id) - sizeof(dest);
	memcpy(destptr, &dest, sizeof(dest));
	csp_bin_sem_wait(&drv->tx_wait, CSP_MAX_TIMEOUT); /* Using ZMQ in thread safe manner*/
	int result = zmq_send(drv->publisher, destptr, length + sizeof(packet->id) + sizeof(dest), 0);
	csp_bin_sem_post(&drv->tx_wait); /* Release tx semaphore */
	if (result < 0) {
		csp_log_error("ZMQ send error: %u %s\r\n", result, zmq_strerror(zmq_errno()));
	}

	csp_buffer_free(packet);

	return CSP_ERR_NONE;

}

CSP_DEFINE_TASK(csp_zmqhub_task) {

	zmq_driver_t * drv = param;
	csp_packet_t * packet;
	const uint32_t HEADER_SIZE = (sizeof(packet->id) + sizeof(uint8_t));

	csp_log_info("RX %s started", drv->interface.name);

	while(1) {
		zmq_msg_t msg;
		assert(zmq_msg_init_size(&msg, CSP_ZMQ_MTU + HEADER_SIZE) == 0);

		// Receive data
		if (zmq_msg_recv(&msg, drv->subscriber, 0) < 0) {
			csp_log_error("RX %s: %s", drv->interface.name, zmq_strerror(zmq_errno()));
			continue;
		}

		unsigned int datalen = zmq_msg_size(&msg);
		if (datalen < HEADER_SIZE) {
			csp_log_warn("RX %s: Too short datalen: %u - expected min %u bytes", drv->interface.name, datalen, HEADER_SIZE);
			zmq_msg_close(&msg);
			continue;
		}

		// Create new csp packet
		packet = csp_buffer_get(datalen - HEADER_SIZE);
		if (packet == NULL) {
			csp_log_warn("RX %s: Failed to get csp_buffer(%u)", drv->interface.name, datalen);
			zmq_msg_close(&msg);
			continue;
		}

		// Copy the data from zmq to csp
		const uint8_t * rx_data = zmq_msg_data(&msg);

		// First byte is the MAC (via) address
		++rx_data;
		--datalen;

		// Remaining is CSP header and payload
		memcpy(&packet->id, rx_data, datalen);
		packet->length = (datalen - sizeof(packet->id));

		// Route packet
		csp_qfifo_write(packet, &drv->interface, NULL);

		zmq_msg_close(&msg);
	}

	return CSP_TASK_RETURN;

}

int csp_zmqhub_make_endpoint(const char * host, uint16_t port, char * buf, size_t buf_size) {
	int res = snprintf(buf, buf_size, "tcp://%s:%u", host, port);
	if ((res < 0) || (res >= (int)buf_size)) {
		buf[0] = 0;
		return CSP_ERR_NOMEM;
	}
	return CSP_ERR_NONE;
}

int csp_zmqhub_init(uint8_t addr, const char * host) {
	char pub[100];
	csp_zmqhub_make_endpoint(host, CSP_ZMQPROXY_SUBSCRIBE_PORT, pub, sizeof(pub));

	char sub[100];
	csp_zmqhub_make_endpoint(host, CSP_ZMQPROXY_PUBLISH_PORT, sub, sizeof(sub));

	return csp_zmqhub_init_w_endpoints(addr, pub, sub);
}

int csp_zmqhub_init_w_endpoints(uint8_t addr,
                                const char * publisher_endpoint,
				const char * subscriber_endpoint) {

	uint8_t * rxfilter = NULL;
	unsigned int rxfilter_count = 0;

	if (addr != CSP_NODE_MAC) { // != 255
		rxfilter = &addr;
		rxfilter_count = 1;
	}

	return csp_zmqhub_init_w_name_endpoints_rxfilter(CSP_ZMQHUB_IF_NAME,
							 rxfilter, rxfilter_count,
							 publisher_endpoint,
							 subscriber_endpoint,
							 NULL);
}

int csp_zmqhub_init_w_name_endpoints_rxfilter(const char * name,
                                              const uint8_t rxfilter[], unsigned int rxfilter_count,
                                              const char * publish_endpoint,
                                              const char * subscribe_endpoint,
                                              csp_iface_t ** return_interface) {

	zmq_driver_t * drv = csp_malloc(sizeof(*drv));
	assert(drv);
	memset(drv, 0, sizeof(*drv));

	char * alloc_name = csp_malloc(strlen(name) + 1);
	drv->interface.name = alloc_name;
	assert(alloc_name);
	strcpy(alloc_name, name);
	drv->interface.driver_data = drv;
	drv->interface.nexthop = csp_zmqhub_tx;
	drv->interface.mtu = CSP_ZMQ_MTU; // there is actually no 'max' MTU on ZMQ, but assuming the other end is based on the same code

	drv->context = zmq_ctx_new();
	assert(drv->context);

	csp_log_info("INIT %s: pub(tx): [%s], sub(rx): [%s], rx filters: %u",
		     drv->interface.name, publish_endpoint, subscribe_endpoint, rxfilter_count);

	/* Publisher (TX) */
	drv->publisher = zmq_socket(drv->context, ZMQ_PUB);
	assert(drv->publisher);

	/* Subscriber (RX) */
	drv->subscriber = zmq_socket(drv->context, ZMQ_SUB);
	assert(drv->subscriber);

	if (rxfilter && rxfilter_count) {
		for (unsigned int i = 0; i < rxfilter_count; ++i, ++rxfilter) {
			assert(zmq_setsockopt(drv->subscriber, ZMQ_SUBSCRIBE, rxfilter, 1) == 0);
		}
	} else {
		assert(zmq_setsockopt(drv->subscriber, ZMQ_SUBSCRIBE, NULL, 0) == 0);
	}

	/* Connect to server */
	assert(zmq_connect(drv->publisher, publish_endpoint) == 0);
	assert(zmq_connect(drv->subscriber, subscribe_endpoint) == 0);

	/* ZMQ isn't thread safe, so we add a binary semaphore to wait on for tx */
	assert(csp_bin_sem_create(&drv->tx_wait) == CSP_SEMAPHORE_OK);

	/* Start RX thread */
	assert(csp_thread_create(csp_zmqhub_task, drv->interface.name, 20000, drv, 0, &drv->rx_thread) == 0);

	/* Register interface */
	csp_iflist_add(&drv->interface);

	if (return_interface) {
		*return_interface = &drv->interface;
	}

	return CSP_ERR_NONE;

}
