
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
	char * promisc;
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

	int addr = atoi(data->addr);

	/* If dfl_addr is passed, we can either readout or override */
	if ((dfl_addr != NULL) && (data->is_dfl)) {
		if (*dfl_addr == 0) {
			*dfl_addr = addr;
		} else {
			addr = *dfl_addr;
		}
	}

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
		};
		int error = csp_usart_open_and_add_kiss_interface(&conf, data->name, addr, &iface);
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
		memset(iface, 0, sizeof(csp_iface_t));
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

		int promisc = 0;
		if (data->promisc) {
			promisc = (strcmp("true", data->promisc) == 0) ? 1 : 0;
		}

		csp_zmqhub_init_filter2(data->name, data->server, addr, atoi(data->netmask), promisc, &iface, NULL, CSP_ZMQPROXY_SUBSCRIBE_PORT, CSP_ZMQPROXY_PUBLISH_PORT);

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

		int error = csp_can_socketcan_open_and_add_interface(data->device, data->name, addr, 1000000, true, &iface);
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

	iface->addr = addr;
	iface->netmask = atoi(data->netmask);
	iface->name = strdup(data->name);
	iface->is_default = (data->is_dfl) ? 1 : 0;

	csp_print("  %s addr: %u netmask %u %s\n", iface->name, iface->addr, iface->netmask, (iface->is_default) ? "DFL" : "");

}

static void csp_yaml_key_value(struct data_s * data, char * key, char * value) {

	if (strcmp(key, "name") == 0) {
		data->name = strdup(value);
	} else if (strcmp(key, "driver") == 0) {
		data->driver = strdup(value);
	} else if (strcmp(key, "device") == 0) {
		data->device = strdup(value);
	} else if (strcmp(key, "addr") == 0) {
		data->addr = strdup(value);
	} else if (strcmp(key, "netmask") == 0) {
		data->netmask = strdup(value);
	} else if (strcmp(key, "server") == 0) {
		data->server = strdup(value);
	} else if (strcmp(key, "default") == 0) {
		data->is_dfl = strdup(value);
	} else if (strcmp(key, "baudrate") == 0) {
		data->baudrate = strdup(value);
	} else if (strcmp(key, "source") == 0) {
		data->source = strdup(value);
	} else if (strcmp(key, "destination") == 0) {
		data->destination = strdup(value);
	} else if (strcmp(key, "listen_port") == 0) {
		data->listen_port = strdup(value);
	} else if (strcmp(key, "remote_port") == 0) {
		data->remote_port = strdup(value);
	} else if (strcmp(key, "promisc") == 0) {
		data->promisc = strdup(value);
	} else {
		csp_print("Unkown key %s\n", key);
	}
}

void csp_yaml_init(char * filename, unsigned int * dfl_addr) {

    struct data_s data;

	csp_print("  Reading config from %s\n", filename);
	FILE * file = fopen(filename, "rb");
	if (file == NULL) {
		csp_print("  ERROR: failed to find CSP config file\n");
		return;
	}

	yaml_parser_t parser;
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, file);
	yaml_event_t event;

	yaml_parser_parse(&parser, &event);
	if (event.type != YAML_STREAM_START_EVENT) {
		yaml_event_delete(&event);
		yaml_parser_delete(&parser);
		return;
	}
	yaml_event_delete(&event);

	yaml_parser_parse(&parser, &event);
	if (event.type != YAML_DOCUMENT_START_EVENT) {
		yaml_event_delete(&event);
		yaml_parser_delete(&parser);
		return;
	}
	yaml_event_delete(&event);

	yaml_parser_parse(&parser, &event);
	if (event.type != YAML_SEQUENCE_START_EVENT) {
		yaml_event_delete(&event);
		yaml_parser_delete(&parser);
		return;
	}
	yaml_event_delete(&event);

	while (1) {

		yaml_parser_parse(&parser, &event);

		if (event.type == YAML_SEQUENCE_END_EVENT) {
			yaml_event_delete(&event);
			break;
		}

		if (event.type == YAML_MAPPING_START_EVENT) {
			csp_yaml_start_if(&data);
			yaml_event_delete(&event);
			continue;
		}

		if (event.type == YAML_MAPPING_END_EVENT) {
			csp_yaml_end_if(&data, dfl_addr);
			yaml_event_delete(&event);
			continue;
		}

		if (event.type == YAML_SCALAR_EVENT) {
			
			/* Got key, parse the value too */
			yaml_event_t event_val;
			yaml_parser_parse(&parser, &event_val);
			csp_yaml_key_value(&data, (char *) event.data.scalar.value, (char *) event_val.data.scalar.value);
			yaml_event_delete(&event_val);

			yaml_event_delete(&event);

			continue;
		}
	}

	/* Cleanup libyaml */
	yaml_parser_delete(&parser);

	/* Go through list of potentially allocated strings. Tedious cleanup */
	free(data.name);
	free(data.driver);
	free(data.device);
	free(data.addr);
	free(data.netmask);
	free(data.server);
	free(data.is_dfl);
	free(data.baudrate);
	free(data.source);
	free(data.destination);
	free(data.listen_port);
	free(data.remote_port);
	free(data.promisc);

}
