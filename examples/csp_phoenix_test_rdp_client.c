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

#include <time.h>
#include <sys/times.h>
#include <sys/threads.h>
#include <posix/utils.h>
#include <pthread.h>

#define STACK_SZ 8192

char router_stack[STACK_SZ];
char client_stack[STACK_SZ];

/* Server port, the port the server listens on for incoming connections from the client. */
#define MY_SERVER_PORT 10

static uint8_t server_address = 1;
static uint8_t client_address = 2;

#define TEST_BD 1000000
#define TEST_LEN 1024 * 1024 / 4

grlibCan_dev_t * device;
csp_iface_t iface;
csp_can_interface_data_t ifdata;

int data[TEST_LEN];

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

/* Client task sending requests to server task */
void client(void * param) {
	(void)param;

	csp_print("Client task started\n");

	csp_conn_t * conn = NULL;
	while (conn == NULL) {
		conn = csp_connect(CSP_PRIO_NORM, server_address, MY_SERVER_PORT, CSP_MAX_TIMEOUT, CSP_O_RDP);
		if (conn == NULL)
			printf("Failed to establish RDP connection\n");
	}

	printf("Connection with server established\n");

	for(int i = 0; i < TEST_LEN; i ++){
		data[i] = i;
	}

	if(csp_sfp_send(conn, data, TEST_LEN * sizeof(int), 128, CSP_MAX_TIMEOUT) < 0){
		printf("Sending failed\n");
	}

	printf("Managed to send %ld bytes\n", TEST_LEN * sizeof(int));
	csp_close(conn);

	endthread();
}
/* End of client task */

int main(int argc, char ** argv) {
	priority(4);

	csp_print("Initialising CSP\n");

	/* Init CSP */
	csp_init();

	grlibCan_dev_t devices[10];

	int detectedDevices = grlibCan_queryForDevices(devices);
	printf("Detected %d CAN devices\n", detectedDevices);

	if (detectedDevices == 0)
		return EXIT_FAILURE;

	device = &devices[1];

	grlibCan_initDevices(device, 1);
	grlibCan_applyDefConf(device);

	can_context_t context;
	csp_iface_t * iface = csp_can_grlibCan_init(&context, device, client_address, TEST_BD, true);

	if (iface == NULL) {
		printf("Failed to register interface\n");
	}

	csp_print("Interfaces\r\n");
	csp_iflist_print();

	csp_print("Adding routing information\n");
	csp_rtable_set(1, 32, iface, CSP_NO_VIA_ADDRESS);
	csp_rtable_print();

	/* Start router */
	printf("Starting router thread\n");
	beginthread(task_router, 4, router_stack, STACK_SZ, NULL);

	csp_print("Starting client thread\n");
	beginthread(client, 4, client_stack, STACK_SZ, NULL);

	for (;;) {
		sleep(5);
	}

	return 0;
}
