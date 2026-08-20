#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <zmq.h>
#include <assert.h>
#include <pthread.h>

#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/interfaces/csp_if_zmqhub.h>

int debug = 0;
const char * sub_str = "tcp://0.0.0.0:6000";
const char * pub_str = "tcp://0.0.0.0:7000";
char * logfile_name = NULL;
FILE * logfile;

static size_t max_raw_frame_length(void) {

	return csp_id_get_header_size() + ((csp_conf.version == 1) ? 1 : 0) + CSP_ZMQ_MTU;
}

static void * task_capture(void * ctx) {

    int ret;

	csp_print("Capture/logging task listening on %s\n", sub_str);

	/* Subscriber (RX) */
	void * subscriber = zmq_socket(ctx, ZMQ_SUB);
	ret = zmq_connect(subscriber, pub_str);
	if (ret < 0) {
		perror("Unable to connect");
		exit(1);
    }
	ret = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);
	if (ret < 0) {
		perror("Failed to call setsockopt");
		exit(1);
    }


	if (logfile_name) {
		logfile = fopen(logfile_name, "a+");
		if (logfile == NULL) {
			csp_print("Unable to open logfile %s\n", logfile_name);
			exit(-1);
		}
	}

	while (1) {
		zmq_msg_t msg;
		zmq_msg_init_size(&msg, 1024);

		/* Receive data */
		if (zmq_msg_recv(&msg, subscriber, 0) < 0) {
			zmq_msg_close(&msg);
			csp_print("ZMQ: %s\n", zmq_strerror(zmq_errno()));
			continue;
		}

		size_t datalen = zmq_msg_size(&msg);
		const size_t header_size = csp_id_get_header_size() + ((csp_conf.version == 1) ? 1 : 0);
		const size_t max_frame_length = max_raw_frame_length();
		if (datalen < header_size || datalen > max_frame_length) {
			csp_print("ZMQ: Invalid datalen: %zu - expected %zu to %zu bytes\n", datalen, header_size, max_frame_length);
			zmq_msg_close(&msg);
			continue;
		}

		uint8_t * rx_data = csp_zmqhub_fixup_cspv1_del_dest_addr(zmq_msg_data(&msg), &datalen);
		csp_id_t id = csp_id_extract_fixup_cspv1(rx_data);
		const size_t payload_length = datalen - csp_id_get_header_size();

		/* Print header data */
		csp_print("Packet: Src %u, Dst %u, Dport %u, Sport %u, Pri %u, Flags 0x%02X, Size %zu\n",
			   id.src, id.dst, id.dport, id.sport, id.pri, id.flags, payload_length);


		if (logfile) {
			const char * delimiter = "--------\n";
			fputs(delimiter, logfile);
			fwrite(rx_data, datalen, 1, logfile);
			fflush(logfile);
		}

		zmq_msg_close(&msg);
	}
}

int main(int argc, char ** argv) {

    int ret;
	csp_conf.version = 2;

	int opt;
	while ((opt = getopt(argc, argv, "dhv:s:p:f:")) != -1) {
		switch (opt) {
			case 'd':
				debug = 1;
				break;
			case 'v':
				csp_conf.version = atoi(optarg);
				break;
			case 's':
				sub_str = optarg;
				break;
			case 'p':
				pub_str = optarg;
				break;
			case 'f':
				logfile_name = optarg;
				break;
			default:
				csp_print(
					"Usage:\n"
					" -d \t\tEnable debug\n"
					" -v VERSION\tcsp version\n"
					" -s SUB_STR\tsubscriber port: tcp://localhost:7000\n"
					" -p PUB_STR\tpublisher  port: tcp://localhost:6000\n"
					" -f LOGFILE\tLog to this file\n");
				exit(1);
				break;
		}
	}

	void * ctx = zmq_ctx_new();
	assert(ctx);

	void * frontend = zmq_socket(ctx, ZMQ_XSUB);
	assert(frontend);
    ret = zmq_bind(frontend, sub_str);
	if (ret < 0) {
		perror("Failed to bind to ZMQ_XSUB");
		return 1;
	}
	csp_print("Subscriber task listening on %s\n", sub_str);

	void * backend = zmq_socket(ctx, ZMQ_XPUB);

    ret = zmq_bind(backend, pub_str);
	if (ret < 0) {
		perror("Failed to bind to ZMQ_XPUB");
		return 1;
	}
	csp_print("Publisher task listening on %s\n", pub_str);

	pthread_t capworker;
	pthread_create(&capworker, NULL, task_capture, ctx);

	zmq_proxy(frontend, backend, NULL);

	csp_print("Closing ZMQproxy");
	zmq_ctx_destroy(ctx);

	return 0;
}
