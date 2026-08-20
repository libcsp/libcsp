#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <zmq.h>
#include <assert.h>
#include <errno.h>
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

static int close_msg(zmq_msg_t * msg) {

	int ret = zmq_msg_close(msg);
	if (ret != 0) {
		csp_print("ZMQ: failed to close message: %s\n", zmq_strerror(zmq_errno()));
		return -1;
	}
	return 0;
}

static int recv_msg(zmq_msg_t * msg, void * subscriber) {

	int ret;
	do {
		ret = zmq_msg_recv(msg, subscriber, 0);
	} while ((ret < 0) && (zmq_errno() == EINTR));
	return ret;
}

static int drain_multipart(void * subscriber) {

	int more = 1;
	while (more) {
		zmq_msg_t part;
		int ret = zmq_msg_init(&part);
		if (ret != 0) {
			return -1;
		}
		ret = recv_msg(&part, subscriber);
		if (ret < 0) {
			csp_print("ZMQ: %s\n", zmq_strerror(zmq_errno()));
			close_msg(&part);
			return -1;
		}
		more = zmq_msg_more(&part);
		ret = close_msg(&part);
		if (ret != 0) {
			return -1;
		}
	}
	return 0;
}

static int set_max_msg_size(void * socket, size_t max_frame_length) {

	const int64_t max_msg_size = max_frame_length;
	return zmq_setsockopt(socket, ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
}

static void * task_capture(void * ctx) {

    int ret;

	csp_print("Capture/logging task listening on %s\n", sub_str);

	/* Subscriber (RX) */
	void * subscriber = zmq_socket(ctx, ZMQ_SUB);
	if (subscriber == NULL) {
		return NULL;
	}
	ret = set_max_msg_size(subscriber, UINT16_MAX);
	if (ret != 0) {
		perror("Failed to set maximum message size");
		zmq_close(subscriber);
		return NULL;
	}
	ret = zmq_connect(subscriber, pub_str);
	if (ret < 0) {
		perror("Unable to connect");
		zmq_close(subscriber);
		return NULL;
    }
	ret = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);
	if (ret < 0) {
		perror("Failed to call setsockopt");
		zmq_close(subscriber);
		return NULL;
    }


	if (logfile_name) {
		logfile = fopen(logfile_name, "a+");
		if (logfile == NULL) {
			csp_print("Unable to open logfile %s\n", logfile_name);
			zmq_close(subscriber);
			return NULL;
		}
	}

	while (1) {
		zmq_msg_t msg;
		ret = zmq_msg_init(&msg);
		if (ret != 0) {
			csp_print("ZMQ: failed to initialize message: %s\n", zmq_strerror(zmq_errno()));
			continue;
		}

		/* Receive data */
		ret = recv_msg(&msg, subscriber);
		if (ret < 0) {
			csp_print("ZMQ: %s\n", zmq_strerror(zmq_errno()));
			close_msg(&msg);
			break;
		}

		/* A CSP frame is carried in one single-part ZMQ message. */
		if (zmq_msg_more(&msg)) {
			csp_print("ZMQ: multipart messages are not supported\n");
			ret = close_msg(&msg);
			if (ret != 0) {
				break;
			}
			ret = drain_multipart(subscriber);
			if (ret != 0) {
				break;
			}
			continue;
		}

		size_t datalen = zmq_msg_size(&msg);
		const size_t header_size = csp_id_get_header_size() + ((csp_conf.version == 1) ? 1 : 0);
		const size_t max_frame_length = max_raw_frame_length();
		if (datalen < header_size || datalen > max_frame_length) {
			csp_print("ZMQ: Invalid datalen: %zu - expected %zu to %zu bytes\n", datalen, header_size, max_frame_length);
			ret = close_msg(&msg);
			if (ret != 0) {
				break;
			}
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

		ret = close_msg(&msg);
		if (ret != 0) {
			break;
		}
	}

	zmq_close(subscriber);
	return NULL;
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
	/* Forward frames for any supported packet-buffer configuration. */
    ret = set_max_msg_size(frontend, UINT16_MAX);
	if (ret < 0) {
		perror("Failed to set maximum message size");
		zmq_close(frontend);
		zmq_ctx_destroy(ctx);
		return 1;
	}
    ret = zmq_bind(frontend, sub_str);
	if (ret < 0) {
		perror("Failed to bind to ZMQ_XSUB");
		zmq_close(frontend);
		zmq_ctx_destroy(ctx);
		return 1;
	}
	csp_print("Subscriber task listening on %s\n", sub_str);

	void * backend = zmq_socket(ctx, ZMQ_XPUB);
	if (backend == NULL) {
		zmq_close(frontend);
		zmq_ctx_destroy(ctx);
		return 1;
	}

    ret = zmq_bind(backend, pub_str);
	if (ret < 0) {
		perror("Failed to bind to ZMQ_XPUB");
		zmq_close(backend);
		zmq_close(frontend);
		zmq_ctx_destroy(ctx);
		return 1;
	}
	csp_print("Publisher task listening on %s\n", pub_str);

	pthread_t capworker;
	ret = pthread_create(&capworker, NULL, task_capture, ctx);
	if (ret != 0) {
		zmq_close(backend);
		zmq_close(frontend);
		zmq_ctx_destroy(ctx);
		return 1;
	}

	zmq_proxy(frontend, backend, NULL);

	csp_print("Closing ZMQproxy");
	zmq_ctx_shutdown(ctx);
	pthread_join(capworker, NULL);
	zmq_close(backend);
	zmq_close(frontend);
	zmq_ctx_destroy(ctx);

	return 0;
}
