#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/drivers/usart.h>

#define SERVER_ADDR 1
#define SERVER_PORT 10

#define CLIENT_ADDR 2
#define DEVICE_NAME "/tmp/pty2"

int main(int argc, char * argv[])
{
	csp_usart_conf_t conf = {
		.device = DEVICE_NAME,
		.baudrate = 115200,
		.databits = 8,
		.stopbits = 1,
		.paritysetting = 0,
	};
	csp_iface_t * iface;
	csp_conn_t * conn;
	csp_packet_t * packet;
	int ret;

	/* init */
    csp_init();

	/* open */
	ret = csp_usart_open_and_add_kiss_interface(&conf, CSP_IF_KISS_DEFAULT_NAME, CLIENT_ADDR, &iface);
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
