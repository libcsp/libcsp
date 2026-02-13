#include <unistd.h>
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


	/* Allocated 'raw' CSP packet */
	csp_packet_t * packet = malloc(1024);
	assert(packet != NULL);

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
		if (datalen < 5) {
			csp_print("ZMQ: Too short datalen: %u\n", datalen);
			while (zmq_msg_recv(&msg, subscriber, ZMQ_NOBLOCK) > 0)
				zmq_msg_close(&msg);
			continue;
		}

		uint8_t * rx_data = csp_zmqhub_fixup_cspv1_del_dest_addr(zmq_msg_data(&msg), &datalen);

		/* Copy to packet */
		csp_id_setup_rx(packet);
		memcpy(packet->frame_begin, rx_data, datalen);
		packet->frame_length = datalen;

		/* Parse header */
		csp_id_strip_fixup_cspv1(packet);

		/* Print header data */
		csp_print("Packet: Src %u, Dst %u, Dport %u, Sport %u, Pri %u, Flags 0x%02X, Size %" PRIu16 "\n",
			   packet->id.src, packet->id.dst, packet->id.dport,
			   packet->id.sport, packet->id.pri, packet->id.flags, packet->length);


		if (logfile) {
			const char * delimiter = "--------\n";
			fwrite(delimiter, sizeof(delimiter), 1, logfile);
			fwrite(packet->frame_begin, packet->frame_length, 1, logfile);
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
