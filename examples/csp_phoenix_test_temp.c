#include <csp/csp_debug.h>
#include <string.h>
#include <stdlib.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/interfaces/csp_if_zmqhub.h>

#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/threads.h>
#include <posix/utils.h>

#include "../src/csp_io.h"

#include "grlib-can-core.h"

#define STACK_SZ 4096

char router_stack[STACK_SZ];
char rx_stack1[STACK_SZ];
char rx_stack2[STACK_SZ];
char server_stack[STACK_SZ];
char client_stack[STACK_SZ];

/* Server port, the port the server listens on for incoming connections from the client. */
#define MY_SERVER_PORT 10

/* Commandline options */
static uint8_t server_address = 1;
static uint8_t client_address = 2;

/* test mode, used for verifying that host & client can exchange packets over the loopback interface */
static unsigned int server_received = 0;

grlibCan_dev_t * device1;
grlibCan_dev_t * device2;

csp_iface_t iface1;
csp_iface_t iface2;

csp_can_interface_data_t ifdata1;
csp_can_interface_data_t ifdata2;

/* Server task - handles requests from clients */
void server(void * param) {

	(void)param;

	csp_print("Server task started\n");

	/* Create socket with no specific socket options, e.g. accepts CRC32, HMAC, etc. if enabled during compilation */
	csp_socket_t sock = {0};

	/* Bind socket to all ports, e.g. all incoming connections will be handled here */
	csp_bind(&sock, CSP_ANY);
	/* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued */
	csp_listen(&sock, 10);

	/* Wait for connections and then process packets on the connection */
	while (1) {

		/* Wait for a new connection, 10000 mS timeout */
		csp_conn_t * conn;
		if ((conn = csp_accept(&sock, 10000)) == NULL) {
			/* timeout */
			continue;
		}

		/* Read packets on connection, timout is 100 mS */
		csp_packet_t * packet;
		while ((packet = csp_read(conn, 50)) != NULL) {
			switch (csp_conn_dport(conn)) {
				case MY_SERVER_PORT:
					/* Process packet here */
					csp_print("Packet received on MY_SERVER_PORT: %s\n", (char *)packet->data);
					csp_buffer_free(packet);
					++server_received;
					break;

				default:
					/* Call the default CSP service handler, handle pings, buffer use, etc. */
					csp_service_handler(packet);
					break;
			}
		}

		/* Close current connection */
		csp_close(conn);
	}

	for (;;) {
		sleep(10);
	}
}
/* End of server task */

/* Router task */
void task_router(void * param) {

	(void)param;

	/* Here there be routing */
	while (1) {
		csp_route_work();
	}

	for (;;) {
		sleep(10);
	}
}
/* End of router task*/

uint32_t reversedUint32_t(uint32_t value) {
	uint32_t ret = 0;
	for (int i = 0; i < 32; i++) {
		ret |= (((value & (1 << i)) >> i) << (31 - i));
	}
	return ret;
}

/* RX task - reading packets and checking if it should be enqueued for CSP */
void rx_task(void * param) {
	int num = *(int *)param;

	csp_iface_t *iface = (num == 1 ? &iface1 : &iface2);

	grlibCan_msg_t frame;
	grlibCan_dev_t *dev = (num == 1 ? device1 : device2);

	while (1) {
		int ret = grlibCan_recvSync(dev, &frame, 1);
		if (ret == 0)
			continue;

		// printf("ID: %x\n", frame.frame.head & ~(1 << 31));
		// printf("DLC: %x\n", frame.frame.stat >> 28);

		// for (int i = 0; i < frame.frame.stat >> 28; i++) {
		// 	printf("%d: %x\n", i, frame.frame.payload[i]);
		// }

		int ret2 = csp_can_rx(iface, frame.frame.head & ~(1 << 31), frame.frame.payload, frame.frame.stat >> 28, NULL);
		printf("Can%d: csp can rx %d\n", num, ret2);
	}

	/* This point should never be reached */
}

/* Client task sending requests to server task */
void client(void * param) {

	(void)param;

	csp_print("Client task started\n");

	while (1) {

		usleep(1000000);

		/* Send ping to server, timeout 1000 mS, ping size 100 bytes */
		int result = csp_ping(server_address, 1000, 100, CSP_O_NONE);
		csp_print("Ping address: %u, result %d [mS]\n", server_address, result);
		(void)result;
	}

	for (;;) {
		sleep(10);
	}
}
/* End of client task */

int grlibCan_txWrapper1(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc) {
	printf("Wrapper 1\n");
	grlibCan_msg_t packet;

	packet.frame.head = id | (1 << 31);
	packet.frame.stat = dlc << 28;

	memcpy(packet.frame.payload, data, dlc);

	return grlibCan_transmitSync(device1, &packet, 1);
}

int grlibCan_txWrapper2(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc) {
	printf("Wrapper 2\n");
	grlibCan_msg_t packet;

	packet.frame.head = id | (1 << 31);
	packet.frame.stat = dlc << 28;

	memcpy(packet.frame.payload, data, dlc);

	return grlibCan_transmitSync(device2, &packet, 1);
}

int main(int argc, char ** argv) {
	csp_print("Initialising CSP");

	/* Init CSP */
	csp_init();

	grlibCan_dev_t devices[10];

	int detectedDevices = grlibCan_queryForDevices(devices);
	printf("Detected %d CAN devices\n", detectedDevices);

	if (detectedDevices == 0)
		return EXIT_FAILURE;

	device1 = &devices[0];
	device2 = &devices[1];

	grlibCan_initDevices(device1, 1);
	grlibCan_initDevices(device2, 1);

	grlibCan_applyDefConf(device1);
	grlibCan_applyDefConf(device2);

	grlibCan_config_t config;
	grlibCan_copyConfig(device1, &config);
	config.conf &= ~(1 << 6);
	config.nomBdRate = 1e6;
	config.dataBdRate = 1e6;

	grlibCan_applyConfig(device1, &config);
	grlibCan_applyConfig(device2, &config);
	/* Add interface(s) */

	iface1.name = "Can1";
	iface1.netmask = 32;
	iface1.addr = server_address;
	iface1.interface_data = &ifdata1;
	ifdata1.tx_func = grlibCan_txWrapper1;
	ifdata1.pbufs = NULL;

	iface2.name = "Can2";
	iface2.netmask = 32;
	iface2.addr = client_address;
	iface2.interface_data = &ifdata2;
	ifdata2.tx_func = grlibCan_txWrapper2;
	ifdata2.pbufs = NULL;

	csp_can_add_interface(&iface1);
	csp_can_add_interface(&iface2);

	csp_print("Adding routing information\n");
	csp_rtable_print();
	/* Start router */
	beginthread(task_router, 4, router_stack, STACK_SZ, NULL);

	csp_print("Interfaces\r\n");
	csp_iflist_print();

	csp_print("Starting RX thread\n");
	int a = 1;
	beginthread(rx_task, 4, rx_stack1, STACK_SZ, &a);
	a = 2;
	beginthread(rx_task, 4, rx_stack2, STACK_SZ, &a);

	csp_print("Starting server thread\n");
	beginthread(server, 4, server_stack, STACK_SZ, NULL);

	csp_print("Starting client thread\n");
	beginthread(client, 4, client_stack, STACK_SZ, NULL);

	for (;;) {
		sleep(5);
		csp_print("Interfaces\r\n");
		csp_iflist_print();
	}

	return 0;
}
