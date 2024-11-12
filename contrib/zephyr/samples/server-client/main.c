#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <zephyr/logging/log.h>
#include <csp/drivers/can_zephyr.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(csp_sample_server_client);

/* These three functions must be provided in arch specific way */
void router_start(void);
void server_start(void);
void client_start(void);

/* Server port, the port the server listens on for incoming connections from the client. */
#define MY_SERVER_PORT		10

/* Commandline options */
static uint8_t server_address = 255;

/* test mode, used for verifying that host & client can exchange packets over the loopback interface */
static bool test_mode = false;
static unsigned int server_received = 0;

/* Server task - handles requests from clients */
void server(void) {

	LOG_INF("Server task started");

	/* Create socket with no specific socket options, e.g. accepts CRC32, HMAC, etc. if enabled during compilation */
	csp_socket_t sock = {0};

	/* Bind socket to all ports, e.g. all incoming connections will be handled here */
	csp_bind(&sock, CSP_ANY);

	/* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued */
	csp_listen(&sock, 10);

	/* Wait for connections and then process packets on the connection */
	while (1) {

		/* Wait for a new connection, 10000 mS timeout */
		csp_conn_t *conn;
		if ((conn = csp_accept(&sock, 10000)) == NULL) {
			/* timeout */
			continue;
		}

		/* Read packets on connection, timout is 100 mS */
		csp_packet_t *packet;
		while ((packet = csp_read(conn, 50)) != NULL) {
			switch (csp_conn_dport(conn)) {
			case MY_SERVER_PORT:
				/* Process packet here */
				LOG_INF("Packet received on MY_SERVER_PORT: %s", (char *) packet->data);
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

	return;

}
/* End of server task */

/* Client task sending requests to server task */
void client(void) {

	LOG_INF("Client task started");

	unsigned int count = 'A';

	while (1) {

		k_sleep(test_mode ? K_USEC(200000) : K_USEC(1000000));

		/* Send ping to server, timeout 1000 mS, ping size 100 bytes */
		int __maybe_unused result = csp_ping(server_address, 1000, 100, CSP_O_NONE);
		LOG_INF("Ping address: %u, result %d [mS]", server_address, result);

		/* Send reboot request to server, the server has no actual implementation of csp_sys_reboot() and fails to reboot */
		csp_reboot(server_address);
		LOG_INF("reboot system request sent to address: %u", server_address);

		/* Send data packet (string) to server */

		/* 1. Connect to host on 'server_address', port MY_SERVER_PORT with regular UDP-like protocol and 1000 ms timeout */
		csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, server_address, MY_SERVER_PORT, 1000, CSP_O_NONE);
		if (conn == NULL) {
			/* Connect failed */
			LOG_ERR("Connection failed");
			return;
		}

		/* 2. Get packet buffer for message/data */
		csp_packet_t * packet = csp_buffer_get_always();
		if (packet == NULL) {
			/* Could not get buffer element */
			LOG_ERR("Failed to get CSP buffer");
			return;
		}

		/* 3. Copy data to packet */
		memcpy(packet->data, "Hello world ", 12);
		memcpy(packet->data + 12, &count, 1);
		memset(packet->data + 13, 0, 1);
		count++;

		/* 4. Set packet length */
		packet->length = (strlen((char *) packet->data) + 1); /* include the 0 termination */

		/* 5. Send packet */
		csp_send(conn, packet);

		/* 6. Close connection */
		csp_close(conn);
	}

	return;
}
/* End of client task */

/* main - initialization of CSP and start of server/client tasks */
int main(void) {

	int ret;
	uint8_t address = 0;
	const char * kiss_device = NULL;
	const char * rtable = NULL;
	csp_iface_t * can_iface = NULL;

	LOG_INF("Initialising CSP");

	/* Init CSP */
	csp_init();

	/* Start router */
	router_start();

	/* Add interface(s) */
	csp_iface_t * default_iface = NULL;
	if (kiss_device) {
		csp_usart_conf_t conf = {
			.device = kiss_device,
			.baudrate = 115200, /* supported on all platforms */
			.databits = 8,
			.stopbits = 1,
			.paritysetting = 0,
		};
		int error = csp_usart_open_and_add_kiss_interface(&conf, CSP_IF_KISS_DEFAULT_NAME, addr, &default_iface);
		if (error != CSP_ERR_NONE) {
			LOG_ERR("failed to add KISS interface [%s], error: %d", kiss_device, error);
			exit(1);
		}
		default_iface->is_default = 1;
	}

	if (IS_ENABLED(CONFIG_CSP_HAVE_CAN)) {
		/*
		 * In this sample, CAN parameter (address, device, IF name, bitrate) is fixed.
		 * We will improve it so that you can specify each parameter later.
		 * And run as the server, if you want to run as the client, please change the
		 * server address to any address not 255.
		 */
		const char * ifname = "CAN0";
		address = 10;
		server_address = 255;
		const struct device * device = DEVICE_DT_GET(DT_NODELABEL(can0));
		uint32_t bitrate = 1000000;

		/*
		 * In the Zephyr, CSP users can specify the filter settings for any destination
		 * address and mask. In this sample, it is set to filter only packets received
		 * by me. If you want to receive all packets, please change the filter address
		 * and mask. (For example, filter_addr: 0x3FFF, filter_mask: 0x0000)
		 */
		uint16_t filter_addr = address;
		uint16_t filter_mask = 0x3FFF;

		int error = csp_can_open_and_add_interface(device, ifname, address, bitrate,
							   filter_addr, filter_mask, &can_iface);
		if (error != CSP_ERR_NONE) {
			LOG_ERR("failed to add CAN interface [%s], error: %d\n", ifname, error);
			exit(1);
		}
		can_iface->is_default = 1;
		default_iface = can_iface;
	}

	if (IS_ENABLED(CONFIG_CSP_USE_RTABLE)) {
		if (rtable) {
			int error = csp_rtable_load(rtable);
			if (error < 1) {
				LOG_ERR("csp_rtable_load(%s) failed, error: %d", rtable, error);
				exit(1);
			}
		} else if (default_iface) {
			csp_rtable_set(0, 0, default_iface, CSP_NO_VIA_ADDRESS);
		}
	}

	if (!default_iface) {
		/* no interfaces configured - run server and client in process, using loopback interface */
		server_address = address;
		/* run as test mode only use loopback interface */
		test_mode = true;
	}

	/*
	 * In the Zephyr port, we have disabled stdio usage and unified logging with the Zephyr
	 * logging API. As a result, the following functions currently do not print anything.
	 */
	LOG_INF("Connection table");
	csp_conn_print_table();

	LOG_INF("Interfaces");
	csp_iflist_print();

	if (IS_ENABLED(CONFIG_CSP_USE_RTABLE)) {
		LOG_INF("Route table");
		csp_rtable_print();
	}

	/* Start server thread */
	if ((server_address == 255) ||	/* no server address specified, I must be server */
		(default_iface == NULL)) {	/* no interfaces specified -> run server & client via loopback */
		server_start();
	}

	/* Start client thread */
	if ((server_address != 255) ||	/* server address specified, I must be client */
		(default_iface == NULL)) {	/* no interfaces specified -> run server & client via loopback */
		client_start();
	}

	/* Wait for execution to end (ctrl+c) */
	while(1) {
		k_sleep(K_SECONDS(3));

		if (test_mode) {
			/* Test mode is intended for checking that host & client can exchange packets over loopback */
			if (server_received < 5) {
				LOG_INF("Server received %u packets", server_received);
				ret = 1;
				goto end;
			}
			LOG_INF("Server received %u packets", server_received);
			ret = 0;
			goto end;
		}
	}

end:
	if (IS_ENABLED(CONFIG_CSP_HAVE_CAN)) {
		csp_can_stop(can_iface);
	}

	return ret;
}
