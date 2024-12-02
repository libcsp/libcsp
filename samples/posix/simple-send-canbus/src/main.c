#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/drivers/can_socketcan.h>

#define SERVER_ADDR 1
#define SERVER_PORT 10

#define CLIENT_ADDR 2
#define DEVICE_NAME "vcan0"

int main(int argc, char * argv[])
{
	csp_iface_t * iface;
	csp_conn_t * conn;
	csp_packet_t * packet;
	int ret;

	/* init */
	csp_init();

	/* open */
	ret = csp_can_socketcan_open_and_add_interface(DEVICE_NAME, CSP_IF_CAN_DEFAULT_NAME, CLIENT_ADDR, 1000000, true, &iface);
	if (ret != CSP_ERR_NONE) {
		csp_print("failed to open: %d\n", ret);
		return 1;
	}
	iface->is_default = 1;

	/* connect */
	conn = csp_connect(CSP_PRIO_NORM, SERVER_ADDR, SERVER_PORT, 1000, CSP_O_NONE);
	if (conn == NULL) {
		csp_print("Connection failed\n");
		return 1;
	}

	/* prepare data */
	packet = csp_buffer_get_always();
	memcpy(packet->data, "abc", 3);
	packet->length = 3;

	/* send */
	csp_send(conn, packet);

	/* close */
	csp_close(conn);

	return 0;
}
