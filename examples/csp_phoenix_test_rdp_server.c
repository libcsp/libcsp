#include <csp/csp_debug.h>
#include <string.h>
#include <stdlib.h>

#include <csp/csp.h>
#include <csp/drivers/can_grlibCan.h>

#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/threads.h>
#include <posix/utils.h>

#define STACK_SZ 8000

char router_stack[STACK_SZ];
char server_stack[STACK_SZ];

/* Server port, the port the server listens on for incoming connections from the client. */
#define MY_SERVER_PORT 10

/* Commandline options */
static uint8_t server_address = 1;

grlibCan_dev_t * device;
csp_iface_t iface;
csp_can_interface_data_t ifdata;

#define TEST_BD  1000000

/* Server task - handles requests from clients */
void server(void * param) {
	csp_dbg_rdp_print = 0;

	(void)param;

	csp_print("Server task started\n");

	/* Create socket with no specific socket options, e.g. accepts CRC32, HMAC, etc. if enabled during compilation */
	static csp_socket_t sock = {0};

	/* Bind socket to all ports, e.g. all incoming connections will be handled here */
	csp_bind(&sock, CSP_ANY);
	/* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued */
	csp_listen(&sock, 10);

	/* Wait for connections and then process packets on the connection */
	while (1) {

		/* Wait for a new connection, 10000 mS timeout */
		csp_conn_t * conn;
		if ((conn = csp_accept(&sock, CSP_MAX_TIMEOUT)) == NULL) {
			/* timeout */
			continue;
		}

		void *recvData;
		int size;
		if(csp_sfp_recv(conn, &recvData, &size, CSP_MAX_TIMEOUT) < 0){
			printf("Recv failed\n");
		}

		if(recvData == NULL){
			printf("RecvData buffer NULL\n");
		}
		else{
			printf("Received %d ints (%d bytes)\n", (int)(size / sizeof(int)), size);
			int *payload = (int *)recvData;
			for(int i = 0; i < size / sizeof(int); i++){
				if(payload[i] != i){
					printf("Payload[%d] = %d\n", i, payload[i]);
				}
			}
		}

		/* Close current connection */
		printf("Server closing current connection\n");
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

int main(int argc, char ** argv) {
	csp_print("Initialising CSP\n");

	/* Init CSP */
	csp_init();

	grlibCan_dev_t devices[10];

	int detectedDevices = grlibCan_queryForDevices(devices);
	printf("Detected %d CAN devices\n", detectedDevices);

	if (detectedDevices == 0)
		return EXIT_FAILURE;

	device = &devices[0];

	grlibCan_initDevices(device, 1);
	grlibCan_applyDefConf(device);

	can_context_t context;
	csp_iface_t * iface = csp_can_grlibCan_init(&context, device, server_address, TEST_BD, true);

	if (iface == NULL) {
		printf("Failed to register interface\n");
	}

	csp_print("Interfaces\r\n");
	csp_iflist_print();

	csp_print("Adding routing information\n");
	csp_rtable_set(2, 32, iface, CSP_NO_VIA_ADDRESS);
	csp_rtable_print();
	/* Start router */
	printf("Starting router thread\n");
	beginthread(task_router, 4, router_stack, STACK_SZ, NULL);

	csp_print("Starting server thread\n");
	beginthread(server, 4, server_stack, STACK_SZ, NULL);

	for (;;) {
		sleep(5);
		printf("RX irqs: %d\n", device->rxIrqCounter);
	}

	return 0;
}
