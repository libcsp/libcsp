#include <csp/csp_debug.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/interfaces/csp_if_zmqhub.h>

#include "csp_posix_helper.h"

/* Server port, the port the server listens on for incoming connections from the client. */
#define SERVER_PORT		10

/* Commandline options */
static uint8_t server_address = 0;

/* Test mode, check that server & client can exchange packets */
static bool test_mode = false;
static unsigned int server_received = 0;
static unsigned int run_duration_in_sec = 3;

enum DeviceType {
	DEVICE_UNKNOWN,
	DEVICE_CAN,
	DEVICE_KISS,
	DEVICE_ZMQ,
};

#define __maybe_unused __attribute__((__unused__))

/* Server task - handles requests from clients */
void * server(void * param) {

	(void)param;

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

	return NULL;
}
/* End of server task */

static struct option long_options[] = {
    {"kiss-device", required_argument, 0, 'k'},
#if (CSP_HAVE_LIBSOCKETCAN)
	#define OPTION_c "c:"
    {"can-device", required_argument, 0, 'c'},
#else
	#define OPTION_c
#endif
#if (CSP_HAVE_LIBZMQ)
	#define OPTION_z "z:"
    {"zmq-device", required_argument, 0, 'z'},
#else
	#define OPTION_z
#endif
#if (CSP_USE_RTABLE)
	#define OPTION_R "R:"
    {"rtable", required_argument, 0, 'R'},
#else
	#define OPTION_R
#endif
    {"interface-address", required_argument, 0, 'a'},
    {"connect-to", required_argument, 0, 'C'},
	{"protocol-version", required_argument, 0, 'v'},
    {"test-mode", no_argument, 0, 't'},
    {"test-mode-with-sec", required_argument, 0, 'T'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

void print_help(void) {
    csp_print("Usage: csp_server [options]\n");
	if (CSP_HAVE_LIBSOCKETCAN) {
		csp_print(" -c <can-device>  set CAN device\n");
	}
	if (1) {
		csp_print(" -k <kiss-device> set KISS device\n");
	}
	if (CSP_HAVE_LIBZMQ) {
		csp_print(" -z <zmq-device>  set ZeroMQ device\n");
	}
	if (CSP_USE_RTABLE) {
		csp_print(" -R <rtable>      set routing table\n");
	}
	if (1) {
		csp_print(" -a <address>     set interface address\n"
				  " -v <version>     set protocol version\n"
				  " -t               enable test mode\n"
				  " -T <dration>     enable test mode with running time in seconds\n"
				  " -h               print help\n");
	}
}

csp_iface_t * add_interface(enum DeviceType device_type, const char * device_name)
{
    csp_iface_t * default_iface = NULL;

    if (device_type == DEVICE_KISS) {
        csp_usart_conf_t conf = {
			.device = device_name,
            .baudrate = 115200, /* supported on all platforms */
            .databits = 8,
            .stopbits = 1,
            .paritysetting = 0,
		};
        int error = csp_usart_open_and_add_kiss_interface(&conf, CSP_IF_KISS_DEFAULT_NAME, server_address, &default_iface);
        if (error != CSP_ERR_NONE) {
            csp_print("failed to add KISS interface [%s], error: %d\n", device_name, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }

    if (CSP_HAVE_LIBSOCKETCAN && (device_type == DEVICE_CAN)) {
        int error = csp_can_socketcan_open_and_add_interface(device_name, CSP_IF_CAN_DEFAULT_NAME, server_address, 1000000, true, &default_iface);
        if (error != CSP_ERR_NONE) {
            csp_print("failed to add CAN interface [%s], error: %d\n", device_name, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }

    if (CSP_HAVE_LIBZMQ && (device_type == DEVICE_ZMQ)) {
        int error = csp_zmqhub_init(server_address, device_name, 0, &default_iface);
        if (error != CSP_ERR_NONE) {
            csp_print("failed to add ZMQ interface [%s], error: %d\n", device_name, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }

	return default_iface;
}

/* main - initialization of CSP and start of server task */
int main(int argc, char * argv[]) {

	const char * device_name = NULL;
	enum DeviceType device_type = DEVICE_UNKNOWN;
	const char * rtable __maybe_unused = NULL;
	csp_iface_t * default_iface;
    int opt;

	while ((opt = getopt_long(argc, argv, OPTION_c OPTION_z OPTION_R "k:a:v:tT:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
				device_name = optarg;
				device_type = DEVICE_CAN;
                break;
            case 'k':
				device_name = optarg;
				device_type = DEVICE_KISS;
                break;
            case 'z':
				device_name = optarg;
				device_type = DEVICE_ZMQ;
                break;
#if (CSP_USE_RTABLE)
            case 'R':
                rtable = optarg;
                break;
#endif
            case 'a':
                server_address = atoi(optarg);
                break;
			case 'v':
				csp_conf.version = atoi(optarg);
				break;
            case 't':
                test_mode = true;
                break;
            case 'T':
                test_mode = true;
				run_duration_in_sec = atoi(optarg);
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
	if (device_type == DEVICE_UNKNOWN) {
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
	default_iface = add_interface(device_type, device_name);

	/* Setup routing table */
    if (CSP_USE_RTABLE) {
		if (rtable) {
			int error = csp_rtable_load(rtable);
			if (error < 1) {
				csp_print("csp_rtable_load(%s) failed, error: %d\n", rtable, error);
				exit(1);
			}
		} else if (default_iface) {
			csp_rtable_set(0, 0, default_iface, CSP_NO_VIA_ADDRESS);
		}
	}

    csp_print("Connection table\r\n");
    csp_conn_print_table();

    csp_print("Interfaces\r\n");
    csp_iflist_print();

	if (CSP_USE_RTABLE) {
		csp_print("Route table\r\n");
		csp_rtable_print();
	}

    /* Start server thread */
    csp_pthread_create(server);

    /* Wait for execution to end (ctrl+c) */
    while(1) {
        sleep(run_duration_in_sec);

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
