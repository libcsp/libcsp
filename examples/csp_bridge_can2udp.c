#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/interfaces/csp_if_udp.h>

#define DEFAULT_CAN_NAME		"can0"
#define DEFAULT_UDP_ADDRESS		"127.0.0.1"
#define DEFAULT_UDP_REMOTE_PORT	(0)
#define DEFAULT_UDP_LOCAL_PORT	(0)

static struct option long_options[] = {
	{"can", required_argument, 0, 'c'},
	{"remote-address", required_argument, 0, 'a'},
	{"remote-port", required_argument, 0, 'r'},
	{"local-port", required_argument, 0, 'l'},
	{"protocol-version", required_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

/* Overwrite input hook to print packet information */
void csp_input_hook(csp_iface_t * iface, csp_packet_t * packet) {
	char dst_name[] = "UDP";

	if (strncmp(iface->name, "UDP", strlen(iface->name)) == 0) {
		strncpy(dst_name, "CAN", sizeof(dst_name));
	}

	csp_print("%s: %u(%u) --> %s: %u(%u), priority: %u, flags: 0x%02X, size: %" PRIu16 "\n",
			  iface->name, packet->id.src, packet->id.sport,
			  dst_name, packet->id.dst, packet->id.dport,
			  packet->id.pri, packet->id.flags, packet->length);
}

static void print_help(void) {
	csp_print("Usage: csp_bridge_can2udp [options]\n");
	csp_print(" --can                           set CAN interface\n");
	csp_print(" --remote-address <address>      set UDP remote address\n"
			  " --remote-port <port>            set UDP remote port\n"
			  " --local-port <port>             set UDP local port\n"
			  " -v,--protocol-version <version> set protocol version\n"
			  " -h,--help                       print help\n");
}

static csp_iface_t * add_can_iface(const char * can_name)
{
	csp_iface_t * iface = NULL;

	int error = csp_can_socketcan_open_and_add_interface(can_name, CSP_IF_CAN_DEFAULT_NAME,
														 0, 1000000, true, &iface);
	if (error != CSP_ERR_NONE) {
		csp_print("Failed to add CAN interface [%s], error: %d\n", can_name, error);
		exit(1);
	}

	return iface;
}

static csp_iface_t * add_udp_iface(char * address, int lport, int rport)
{
	csp_iface_t * iface = malloc(sizeof(csp_iface_t));
	csp_if_udp_conf_t * conf = malloc(sizeof(csp_if_udp_conf_t));

	conf->host = address;
	conf->lport = lport;
	conf->rport = rport;
	csp_if_udp_init(iface, conf);

	return iface;
}

int main(int argc, char * argv[]) {
	char default_can_name[] = DEFAULT_CAN_NAME;
	char * can_name = default_can_name;

	char default_address[] = DEFAULT_UDP_ADDRESS;
	char * address = default_address;
	int rport = DEFAULT_UDP_REMOTE_PORT;
	int lport = DEFAULT_UDP_LOCAL_PORT;

	csp_iface_t * can_iface;
	csp_iface_t * udp_iface;

	int opt;

	while ((opt = getopt_long(argc, argv, "v:h", long_options, NULL)) != -1) {
		switch (opt) {
			case 'c':
				can_name = optarg;
				break;
			case 'a':
				address = optarg;
				break;
			case 'r':
				rport = atoi(optarg);
				break;
			case 'l':
				lport = atoi(optarg);
				break;
			case 'v':
				csp_conf.version = atoi(optarg);
				break;
			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
			case '?':
				/* Invalid option or missing argument */
				print_help();
				exit(EXIT_FAILURE);
		}
	}

	/* Init CSP */
	csp_init();

	/* Add interfaces */
	can_iface = add_can_iface(can_name);
	udp_iface = add_udp_iface(address, lport, rport);

	/* Set interfaces to bridge */
	csp_bridge_set_interfaces(can_iface, udp_iface);

	/* Print interfaces list */
	csp_iflist_print();

	/* Start bridge */
	while(1) {
		csp_bridge_work();
	}

	return 0;
}
