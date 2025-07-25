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
char rx_stack2[STACK_SZ];
char client_stack[STACK_SZ];

/* Server port, the port the server listens on for incoming connections from the client. */
#define MY_SERVER_PORT 10

/* Commandline options */
static uint8_t server_address = 1;
//static uint8_t client_address = 2;

/* test mode, used for verifying that host & client can exchange packets over the loopback interface */

grlibCan_dev_t * device2;

csp_iface_t iface2;

csp_can_interface_data_t ifdata2;

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


/* RX task - reading packets and checking if it should be enqueued for CSP */
void rx_task(void * param) {
	int num = *(int *)param;

	csp_iface_t *iface = &iface2;

	grlibCan_msg_t frame;
	grlibCan_dev_t *dev = device2;

	while (1) {
		int ret = grlibCan_recvSync(dev, &frame, 1);
		if (ret == 0)
			continue;

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

	device2 = &devices[1];

	grlibCan_initDevices(device2, 1);

	grlibCan_applyDefConf(device2);

	grlibCan_config_t config;
    config.conf &= ~(1 << 6);
	config.nomBdRate = 125000;
	config.dataBdRate = 125000;

	grlibCan_applyConfig(device2, &config);
	/* Add interface(s) */

	// iface2.name = "Can2";
	// iface2.netmask = 32;
	// iface2.addr = client_address;
	// iface2.interface_data = &ifdata2;
	// ifdata2.tx_func = grlibCan_txWrapper2;
	// ifdata2.pbufs = NULL;

	// csp_can_add_interface(&iface2);

	// csp_print("Adding routing information\n");
    // csp_rtable_set(server_address, 32, &iface2, CSP_NO_VIA_ADDRESS);
	// csp_rtable_print();
	// /* Start router */
	// beginthread(task_router, 4, router_stack, STACK_SZ, NULL);

	// csp_print("Interfaces\r\n");
	// csp_iflist_print();

	// csp_print("Starting RX thread\n");
	// int a = 2;
	// beginthread(rx_task, 4, rx_stack2, STACK_SZ, &a);

	// csp_print("Starting client thread\n");
	// beginthread(client, 4, client_stack, STACK_SZ, NULL);

    grlibCan_msg_t msg;
    msg.frame.head = 0xFFF << 20;
    msg.frame.stat = 8 << 28;
    memset(msg.frame.payload, 0xFF, 8);

    printf("Transmiting in em");
    grlibCan_transmitSync(device2, &msg, 1);
    printf("done");

    for(;;){

    }

	return 0;
}
