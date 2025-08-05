#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_udp.h>

#define CLIENT_ADDR 2

#define SERVER_ADDR 1
#define SERVER_PORT 10

#define DEFAULT_UDP_ADDRESS     "127.0.0.1"
#define DEFAULT_UDP_REMOTE_PORT 1501
#define DEFAULT_UDP_LOCAL_PORT  1500

int main(int argc, char * argv[]) {
	csp_conn_t * conn;
	csp_packet_t * packet;
	int ret;

	/* init */
	csp_init();

	/* Interface config */
	csp_iface_t iface;
	csp_if_udp_conf_t conf = {
		.host = DEFAULT_UDP_ADDRESS,
		.lport = DEFAULT_UDP_LOCAL_PORT,
		.rport = DEFAULT_UDP_REMOTE_PORT};

	csp_if_udp_init(&iface, &conf);
	iface.is_default = 1;
	iface.addr = CLIENT_ADDR;

	/* connect */
	conn = csp_connect(CSP_PRIO_NORM, SERVER_ADDR, SERVER_PORT, 1000, CSP_O_NONE);
	if (conn == NULL) {
		csp_print("Connection failed\n");
		return 1;
	}

	/* prepare data */
	packet = csp_buffer_get(0);
	if (packet == NULL) {
		csp_print("Failed to get buffer\n");
		csp_close(conn);
		return 1;
	}
	memcpy(packet->data, "abc", 3);
	packet->length = 3;

	/* send */
	csp_send(conn, packet);

	/* close */
	csp_close(conn);

	return 0;
}
