
#include <csp/csp_yaml.h>
#include <csp/csp_iflist.h>
#include <csp/csp_interface.h>
#include <csp/csp_rtable.h>
#include <csp/csp_id.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include <csp/interfaces/csp_if_can.h>
#include <csp/interfaces/csp_if_lo.h>
#include <csp/interfaces/csp_if_tun.h>
#include <csp/interfaces/csp_if_udp.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/drivers/usart.h>
#include <csp/csp_debug.h>
#include <yaml.h>

struct data_s {
	char * name;
	char * driver;
	char * device;
	char * addr;
	char * netmask;
	char * server;
	char * is_dfl;
	char * baudrate;
	char * source;
	char * destination;
	char * listen_port;
	char * remote_port;
};

static void csp_yaml_start_if(struct data_s * data) {
	memset(data, 0, sizeof(struct data_s));
}

static void csp_yaml_end_if(struct data_s * data, unsigned int * dfl_addr) {
	/* Sanity checks */
	if ((!data->name) || (!data->driver) || (!data->addr) || (!data->netmask)) {
		csp_print("  invalid interface found\n");
		return;
	}

	csp_iface_t * iface;

	/* UART */
    if (strcmp(data->driver, "kiss") == 0) {

		/* Check for valid options */
		if (!data->baudrate) {
			csp_print("no baudrate configured\n");
			return;
		}

		csp_usart_conf_t conf = {
			.device = data->device,
			.baudrate = atoi(data->baudrate),
			.databits = 8,
			.stopbits = 1,
			.paritysetting = 0,
			.checkparity = 0
		};
		int error = csp_usart_open_and_add_kiss_interface(&conf, data->name, &iface);
		if (error != CSP_ERR_NONE) {
			return;
		}

	}

	else if (strcmp(data->driver, "tun") == 0) {

		/* Check for valid options */
		if (!data->source || !data->destination) {
			csp_print("source or destination missing\n");
			return;
		}

		iface = malloc(sizeof(csp_iface_t));
		csp_if_tun_conf_t * ifconf = malloc(sizeof(csp_if_tun_conf_t));
		ifconf->tun_dst = atoi(data->destination);
		ifconf->tun_src = atoi(data->source);

		csp_if_tun_init(iface, ifconf);
	}

	else if (strcmp(data->driver, "udp") == 0) {

		/* Check for valid options */
		if (!data->server || !data->listen_port || !data->remote_port) {
			csp_print("server, listen_port or remote_port missing\n");
			return;
		}

		iface = malloc(sizeof(csp_iface_t));
		csp_if_udp_conf_t * udp_conf = malloc(sizeof(csp_if_udp_conf_t));
		udp_conf->host = data->server;
		udp_conf->lport = atoi(data->listen_port);
		udp_conf->rport = atoi(data->remote_port);
		csp_if_udp_init(iface, udp_conf);

	}

#if (CSP_HAVE_LIBZMQ)
	/* ZMQ */
    else if (strcmp(data->driver, "zmq") == 0) {
		

		/* Check for valid server */
		if (!data->server) {
			csp_print("no server configured\n");
			return;
		}

		csp_zmqhub_init(atoi(data->addr), data->server, 0, &iface);
		strncpy((char *)iface->name, data->name, CSP_IFLIST_NAME_MAX);

		csp_iflist_add(iface);

	}
#endif

#if (CSP_HAVE_LIBSOCKETCAN)
	/* CAN */
	else if (strcmp(data->driver, "can") == 0) {

		/* Check for valid server */
		if (!data->device) {
			csp_print("can: no device configured\n");
			return;
		}

		int error = csp_can_socketcan_open_and_add_interface(data->device, data->name, 1000000, true, &iface);
		if (error != CSP_ERR_NONE) {
			csp_print("failed to add CAN interface [%s], error: %d", data->device, error);
			return;
		}

	}
#endif

    /* Unsupported interface */
	else {
        csp_print("Unsupported driver %s\n", data->driver);
        return;
    }

	iface->addr = atoi(data->addr);

	/* If dfl_addr is passed, we can either readout or override */
	if ((dfl_addr != NULL) && (data->is_dfl)) {
		if (*dfl_addr == 0) {
			*dfl_addr = iface->addr;
		} else {
			iface->addr = *dfl_addr;
		}
	}
	iface->netmask = atoi(data->netmask);
	iface->name = data->name;

	csp_print("  %s addr: %u netmask %u\n", iface->name, iface->addr, iface->netmask);

	if (iface && data->is_dfl) {
		csp_print("  Setting default route to %s\n", iface->name);
		csp_rtable_set(0, 0, iface, CSP_NO_VIA_ADDRESS);
	}
}

static void csp_yaml_key_value(struct data_s * data, char * key, char * value) {

	if (strcmp(key, "name") == 0) {
		data->name = value;
	} else if (strcmp(key, "driver") == 0) {
		data->driver = value;
	} else if (strcmp(key, "device") == 0) {
		data->device = value;
	} else if (strcmp(key, "addr") == 0) {
		data->addr = value;
	} else if (strcmp(key, "netmask") == 0) {
		data->netmask = value;
	} else if (strcmp(key, "server") == 0) {
		data->server = value;
	} else if (strcmp(key, "default") == 0) {
		data->is_dfl = value;
	} else if (strcmp(key, "baudrate") == 0) {
		data->baudrate = value;
	} else if (strcmp(key, "source") == 0) {
		data->source = value;
	} else if (strcmp(key, "destination") == 0) {
		data->destination = value;
	} else if (strcmp(key, "listen_port") == 0) {
		data->listen_port = value;
	} else if (strcmp(key, "remote_port") == 0) {
		data->remote_port = value;
	} else {
		csp_print("Unkown key %s\n", key);
	}
}

void csp_yaml_init(char * filename, unsigned int * dfl_addr) {

    struct data_s data;

	csp_print("  Reading config from %s\n", filename);
	FILE * file = fopen(filename, "rb");
	if (file == NULL)
		return;

	yaml_parser_t parser;
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, file);
	yaml_event_t event;

	yaml_parser_parse(&parser, &event);
	if (event.type != YAML_STREAM_START_EVENT)
		return;

	yaml_parser_parse(&parser, &event);
	if (event.type != YAML_DOCUMENT_START_EVENT)
		return;

	yaml_parser_parse(&parser, &event);
	if (event.type != YAML_SEQUENCE_START_EVENT)
		return;

	while (1) {

		yaml_parser_parse(&parser, &event);

		if (event.type == YAML_SEQUENCE_END_EVENT)
			break;

		if (event.type == YAML_MAPPING_START_EVENT) {
			csp_yaml_start_if(&data);
			continue;
		}

		if (event.type == YAML_MAPPING_END_EVENT) {
			csp_yaml_end_if(&data, dfl_addr);
			continue;
		}

		if (event.type == YAML_SCALAR_EVENT) {
			yaml_char_t * key = event.data.scalar.value;

			yaml_parser_parse(&parser, &event);
			yaml_char_t * value = event.data.scalar.value;

			csp_yaml_key_value(&data, (char *)key, (char *)value);
			continue;
		}
	}
}
