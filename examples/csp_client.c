#include <csp/csp_debug.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/interfaces/csp_if_zmqhub.h>


/* These three functions must be provided in arch specific way */
int router_start(void);
int client_start(void);

/* Server port, the port the server listens on for incoming connections from the client. */
#define SERVER_PORT		10

/* Commandline options */
static uint8_t server_address = 0;
static uint8_t client_address = 0;

/* Test mode, check that server & client can exchange packets */
static bool test_mode = false;
static unsigned int successful_ping = 0;

/* Client task sending requests to server task */
void client(void) {

	csp_print("Client task started\n");

	unsigned int count = 'A';

	while (1) {

		usleep(test_mode ? 200000 : 1000000);

		/* Send ping to server, timeout 1000 mS, ping size 100 bytes */
		int result = csp_ping(server_address, 1000, 100, CSP_O_NONE);
		csp_print("Ping address: %u, result %d [mS]\n", server_address, result);
        // Increment successful_ping if ping was successful
        if (result > 0) {
            ++successful_ping;
        }

		/* Send reboot request to server, the server has no actual implementation of csp_sys_reboot() and fails to reboot */
		csp_reboot(server_address);
		csp_print("reboot system request sent to address: %u\n", server_address);

		/* Send data packet (string) to server */

		/* 1. Connect to host on 'server_address', port SERVER_PORT with regular UDP-like protocol and 1000 ms timeout */
		csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, server_address, SERVER_PORT, 1000, CSP_O_NONE);
		if (conn == NULL) {
			/* Connect failed */
			csp_print("Connection failed\n");
			return;
		}

		/* 2. Get packet buffer for message/data */
		csp_packet_t * packet = csp_buffer_get(100);
		if (packet == NULL) {
			/* Could not get buffer element */
			csp_print("Failed to get CSP buffer\n");
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

static struct option long_options[] = {
#if (CSP_HAVE_LIBSOCKETCAN)
    {"can-device", required_argument, 0, 'c'},
#endif
    {"kiss-device", required_argument, 0, 'k'},
#if (CSP_HAVE_LIBZMQ)
    {"zmq-device", required_argument, 0, 'z'},
#endif
#if (CSP_USE_RTABLE)
    {"rtable", required_argument, 0, 'R'},
#endif
    {"interface-address", required_argument, 0, 'a'},
    {"connect-to", required_argument, 0, 'C'},
    {"test-mode", no_argument, 0, 't'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

void print_help() {
    csp_print("Usage: csp_client [options]\n"
#if (CSP_HAVE_LIBSOCKETCAN)
           " -c <can-device>  set CAN device\n"
#endif
           " -k <kiss-device> set KISS device\n"
#if (CSP_HAVE_LIBZMQ)
           " -z <zmq-device>  set ZeroMQ device\n"
#endif
#if (CSP_USE_RTABLE)
           " -R <rtable>      set routing table\n"
#endif
           " -a <address>     set interface address\n"
           " -C <address>     connect to server at address\n"
           " -t               enable test mode\n"
           " -h               print help\n");
}

/* main - initialization of CSP and start of client task */
int main(int argc, char * argv[]) {

#if (CSP_HAVE_LIBSOCKETCAN)
    const char * can_device = NULL;
#endif
    const char * kiss_device = NULL;
#if (CSP_HAVE_LIBZMQ)
    const char * zmq_device = NULL;
#endif
#if (CSP_USE_RTABLE)
    const char * rtable = NULL;
#endif
    int opt;
    while ((opt = getopt_long(argc, argv, "c:k:z:R:a:C:th", long_options, NULL)) != -1) {
        switch (opt) {
#if (CSP_HAVE_LIBSOCKETCAN)
            case 'c':
                can_device = optarg;
                break;
#endif
            case 'k':
                kiss_device = optarg;
                break;
#if (CSP_HAVE_LIBZMQ)
            case 'z':
                zmq_device = optarg;
                break;
#endif
#if (CSP_USE_RTABLE)
            case 'R':
                rtable = optarg;
                break;
#endif
            case 'a':
                client_address = atoi(optarg);
                break;
            case 'C':
                server_address = atoi(optarg);
                break;
            case 't':
                test_mode = true;
                break;
            case 'h':
				print_help();
				exit(EXIT_SUCCESS);
            case '?':
                // Invalid option or missing argument
				print_help();
                exit(EXIT_FAILURE);
        }
    }

    // If more than one of the interfaces are set, print a message and exit
    if ((kiss_device && can_device) || (kiss_device && zmq_device) || (can_device && zmq_device)) {
        csp_print("Only one of the interfaces can be set.\n");
        print_help();
        exit(EXIT_FAILURE);
    }

    csp_print("Initialising CSP\n");

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
            .checkparity = 0};
        int error = csp_usart_open_and_add_kiss_interface(&conf, CSP_IF_KISS_DEFAULT_NAME,  &default_iface);
        if (error != CSP_ERR_NONE) {
            csp_print("failed to add KISS interface [%s], error: %d\n", kiss_device, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }
#if (CSP_HAVE_LIBSOCKETCAN)
    if (can_device) {
        int error = csp_can_socketcan_open_and_add_interface(can_device, CSP_IF_CAN_DEFAULT_NAME, client_address, 1000000, true, &default_iface);
        if (error != CSP_ERR_NONE) {
            csp_print("failed to add CAN interface [%s], error: %d\n", can_device, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }
#endif
#if (CSP_HAVE_LIBZMQ)
    if (zmq_device) {
        int error = csp_zmqhub_init(client_address, zmq_device, 0, &default_iface);
        if (error != CSP_ERR_NONE) {
            csp_print("failed to add ZMQ interface [%s], error: %d\n", zmq_device, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }
#endif

#if (CSP_USE_RTABLE)
    if (rtable) {
        int error = csp_rtable_load(rtable);
        if (error < 1) {
            csp_print("csp_rtable_load(%s) failed, error: %d\n", rtable, error);
            exit(1);
        }
    } else if (default_iface) {
        csp_rtable_set(0, 0, default_iface, CSP_NO_VIA_ADDRESS);
    }
#endif

    csp_print("Connection table\r\n");
    csp_conn_print_table();

    csp_print("Interfaces\r\n");
    csp_iflist_print();

#if (CSP_USE_RTABLE)
    csp_print("Route table\r\n");
    csp_rtable_print();
#endif

    /* Start client thread */
    client_start();

    /* Wait for execution to end (ctrl+c) */
    while(1) {
        sleep(3);

        if (test_mode) {
            /* Test mode, check that server & client can exchange packets */
            if (successful_ping < 5) {
                csp_print("Client successfully pinged the server %u times\n", successful_ping);
                exit(EXIT_FAILURE);
            }
            csp_print("Client successfully pinged the server %u times\n", successful_ping);
            exit(EXIT_SUCCESS);
        }
    }

    return 0;
}
