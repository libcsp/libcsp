#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include <unistd.h>

#define SERVER_ADDR 1
#define SERVER_PORT 10

#define CLIENT_ADDR 2
#define DEVICE_NAME "localhost"

int main(int argc, char * argv[]) {
	csp_iface_t * iface;
	csp_conn_t * conn;
	csp_packet_t * packet;
	int ret;

	/* init */
	csp_init();

	/* Init zmq interface */
	int error = csp_zmqhub_init(CLIENT_ADDR, DEVICE_NAME, 0, &iface);
	if (error != CSP_ERR_NONE) {
		csp_print("failed to Init ZMQ interface [%s], error: %d\n", DEVICE_NAME, error);
		return 1;
	}
	iface->is_default = 1;

	/* connect */
	conn = csp_connect(CSP_PRIO_NORM, SERVER_ADDR, SERVER_PORT, 1000, CSP_O_NONE);
	if (conn == NULL) {
		csp_print("Connection failed\n");
		return 1;
	}

	/* sleep to have the time to be connected */
	usleep(1000);

	/* prepare data */
	packet = csp_buffer_get(0);
	if (packet == NULL) {
		csp_print("Failed to get buffer\n");
		csp_close(conn);
		return 1;
	}
	memcpy(packet->data, "abc", 4);
	packet->length = 4;

	/* send */
	csp_send(conn, packet);

	/* close */
	csp_close(conn);

	return 0;
}
