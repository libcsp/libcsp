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
int server_start(void);

/* Server port, the port the server listens on for incoming connections from the client. */
#define SERVER_PORT		10

/* Commandline options */
static uint8_t server_address = 0;

/* Test mode, check that server & client can exchange packets */
static bool test_mode = false;
static unsigned int server_received = 0;

/* Server task - handles requests from clients */
void server(void) {

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
		csp_conn_t *conn;
		if ((conn = csp_accept(&sock, 10000)) == NULL) {
			/* timeout */
			continue;
		}

		/* Read packets on connection, timout is 100 mS */
		csp_packet_t *packet;
		while ((packet = csp_read(conn, 50)) != NULL) {
			switch (csp_conn_dport(conn)) {
			case SERVER_PORT:
				/* Process packet here */
				csp_print("Packet received on SERVER_PORT: %s\n", (char *) packet->data);
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
           " -t               enable test mode\n"
           " -h               print help\n");
}

/* main - initialization of CSP and start of server/client tasks */
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
    while ((opt = getopt_long(argc, argv, "c:k:z:R:a:th", long_options, NULL)) != -1) {
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
        int error = csp_can_socketcan_open_and_add_interface(can_device, CSP_IF_CAN_DEFAULT_NAME, server_address, 1000000, true, &default_iface);
        if (error != CSP_ERR_NONE) {
            csp_print("failed to add CAN interface [%s], error: %d\n", can_device, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }
#endif
#if (CSP_HAVE_LIBZMQ)
    if (zmq_device) {
        int error = csp_zmqhub_init(server_address, zmq_device, 0, &default_iface);
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

    /* Start server thread */
    server_start();

    /* Wait for execution to end (ctrl+c) */
    while(1) {
        sleep(3);

        if (test_mode) {
            /* Test mode, check that server & client can exchange packets */
            if (server_received < 5) {
                csp_print("Server received %u packets\n", server_received);
                exit(EXIT_FAILURE);
            }
            csp_print("Server received %u packets\n", server_received);
            exit(EXIT_SUCCESS);
        }
    }

    return 0;
}
