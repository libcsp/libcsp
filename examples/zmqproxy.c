#include <unistd.h>
#include <stdlib.h>
#include <zmq.h>
#include <assert.h>
#include <pthread.h>

#include <csp/csp.h>
#include <csp/arch/csp_system.h>

int csp_id_strip(csp_packet_t * packet);
int csp_id_setup_rx(csp_packet_t * packet);
extern csp_conf_t csp_conf;

int debug = 0;
const char * sub_str = "tcp://0.0.0.0:6000";
const char * pub_str = "tcp://0.0.0.0:7000";
char * logfile_name = NULL;
FILE * logfile;

static void * task_capture(void * ctx) {

    int ret;

	csp_sys_set_color(COLOR_BLUE);
	printf("Capture/logging task listening on %s\n", sub_str);
	csp_sys_set_color(COLOR_RESET);

	/* Subscriber (RX) */
	void * subscriber = zmq_socket(ctx, ZMQ_SUB);
	ret = zmq_connect(subscriber, pub_str);
    assert(ret == 0);
	ret = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);
    assert(ret == 0);

	/* Allocated 'raw' CSP packet */
	csp_packet_t * packet = malloc(1024);
	assert(packet != NULL);

	if (logfile_name) {
		logfile = fopen(logfile_name, "a+");
		if (logfile == NULL) {
			printf("Unable to open logfile %s\n", logfile_name);
			exit(-1);
		}
	}

	while (1) {
		zmq_msg_t msg;
		zmq_msg_init_size(&msg, 1024);

		/* Receive data */
		if (zmq_msg_recv(&msg, subscriber, 0) < 0) {
			zmq_msg_close(&msg);
			csp_log_error("ZMQ: %s\r\n", zmq_strerror(zmq_errno()));
			continue;
		}

		int datalen = zmq_msg_size(&msg);
		if (datalen < 5) {
			csp_log_warn("ZMQ: Too short datalen: %u\r\n", datalen);
			while (zmq_msg_recv(&msg, subscriber, ZMQ_NOBLOCK) > 0)
				zmq_msg_close(&msg);
			continue;
		}

		/* Copy to packet */
		csp_id_setup_rx(packet);
		memcpy(packet->frame_begin, zmq_msg_data(&msg), datalen);
		packet->frame_length = datalen;

		/* Parse header */
		csp_id_strip(packet);

		/* Print header data */
		csp_sys_set_color(COLOR_GREEN);
		printf("Packet: Src %u, Dst %u, Dport %u, Sport %u, Pri %u, Flags 0x%02X, Size %" PRIu16 "\n",
			   packet->id.src, packet->id.dst, packet->id.dport,
			   packet->id.sport, packet->id.pri, packet->id.flags, packet->length);
		csp_sys_set_color(COLOR_RESET);

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
				printf(
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
	assert(ret == 0);
	csp_sys_set_color(COLOR_BLUE);
	printf("Subscriber task listening on %s\n", sub_str);

	void * backend = zmq_socket(ctx, ZMQ_XPUB);
	assert(backend);
    ret = zmq_bind(backend, pub_str);
	assert(ret == 0);
	printf("Publisher task listening on %s\n", pub_str);

	pthread_t capworker;
	pthread_create(&capworker, NULL, task_capture, ctx);

	csp_sys_set_color(COLOR_RESET);

	zmq_proxy(frontend, backend, NULL);

	printf("Closing ZMQproxy");
	zmq_ctx_destroy(ctx);

	return 0;
}
