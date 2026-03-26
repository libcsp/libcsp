#include <unistd.h>
#include <stdlib.h>
#include <zmq.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/interfaces/csp_if_zmqhub.h>

int debug = 0;
const char * sub_str = "tcp://0.0.0.0:6000";
const char * pub_str = "tcp://0.0.0.0:7000";
char * logfile_name = NULL;
FILE * logfile;
volatile sig_atomic_t running = 1;
void * global_ctx = NULL;

/* Statistics */
typedef struct {
    unsigned long packets_received;
    unsigned long packets_sent;
    unsigned long errors;
    unsigned long last_error_time;
} proxy_stats_t;

proxy_stats_t stats = {0};
pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

/* Logging helper with timestamp */
static void log_msg(const char *level, const char *format, ...) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(stderr, "[%s] [%s] ", timestamp, level);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);
}

/* Get ISO8601 timestamp string */
static void get_iso_timestamp(char *buffer, size_t size) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *tm_info = localtime(&ts.tv_sec);
    int len = strftime(buffer, size, "%Y-%m-%dT%H:%M:%S", tm_info);
    snprintf(buffer + len, size - len, ".%06ld", ts.tv_nsec / 1000);
}

/* Log packet in JSON format */
static void log_packet_json(csp_packet_t *packet) {
    if (!logfile) return;

    char timestamp[64];
    get_iso_timestamp(timestamp, sizeof(timestamp));

    fprintf(logfile,
        "{\"type\":\"packet\",\"timestamp\":\"%s\",\"src\":%u,\"dst\":%u,"
        "\"dport\":%u,\"sport\":%u,\"pri\":%u,\"flags\":%u,\"size\":%u}\n",
        timestamp,
        packet->id.src,
        packet->id.dst,
        packet->id.dport,
        packet->id.sport,
        packet->id.pri,
        packet->id.flags,
        packet->length);
    fflush(logfile);
}

/* Signal handler */
static void signal_handler(int signum) {
    const char *signame = "UNKNOWN";
    switch(signum) {
        case SIGINT: signame = "SIGINT"; break;
        case SIGTERM: signame = "SIGTERM"; break;
        case SIGSEGV: signame = "SIGSEGV"; break;
        case SIGABRT: signame = "SIGABRT"; break;
        case SIGBUS: signame = "SIGBUS"; break;
        case SIGFPE: signame = "SIGFPE"; break;
    }

    log_msg("FATAL", "Received signal %d (%s)", signum, signame);
    log_msg("INFO", "Statistics: RX=%lu, TX=%lu, Errors=%lu",
            stats.packets_received, stats.packets_sent, stats.errors);

    if (signum == SIGSEGV || signum == SIGABRT || signum == SIGBUS || signum == SIGFPE) {
        log_msg("FATAL", "Proxy crashed! Check for memory corruption or ZMQ errors");
        signal(signum, SIG_DFL);
        raise(signum);
    }

    running = 0;
    if (global_ctx) {
        zmq_ctx_shutdown(global_ctx);
    }
}

static void * task_capture(void * ctx) {

    int ret;

	log_msg("INFO", "Capture/logging task starting on %s", sub_str);

	/* Subscriber (RX) */
	void * subscriber = zmq_socket(ctx, ZMQ_SUB);
	if (!subscriber) {
		log_msg("ERROR", "Failed to create ZMQ_SUB socket: %s", zmq_strerror(zmq_errno()));
		return NULL;
	}

	ret = zmq_connect(subscriber, pub_str);
	if (ret < 0) {
		log_msg("ERROR", "Unable to connect to %s: %s (errno=%d)",
		        pub_str, zmq_strerror(zmq_errno()), zmq_errno());
		zmq_close(subscriber);
		return NULL;
    }

	ret = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);
	if (ret < 0) {
		log_msg("ERROR", "Failed to set ZMQ_SUBSCRIBE: %s (errno=%d)",
		        zmq_strerror(zmq_errno()), zmq_errno());
		zmq_close(subscriber);
		return NULL;
    }

	log_msg("INFO", "Capture task connected successfully");

	/* Allocated 'raw' CSP packet */
	csp_packet_t * packet = malloc(1024);
	if (packet == NULL) {
		log_msg("FATAL", "Failed to allocate packet buffer");
		zmq_close(subscriber);
		return NULL;
	}

	if (logfile_name) {
		logfile = fopen(logfile_name, "a+");
		if (logfile == NULL) {
			log_msg("ERROR", "Unable to open logfile %s: %s", logfile_name, strerror(errno));
		} else {
			log_msg("INFO", "Logging to file: %s", logfile_name);
			/* Write session start entry */
			char timestamp[64];
			get_iso_timestamp(timestamp, sizeof(timestamp));
			fprintf(logfile, "{\"type\":\"session_start\",\"timestamp\":\"%s\",\"sub_port\":\"%s\",\"pub_port\":\"%s\"}\n",
			        timestamp, sub_str, pub_str);
			fflush(logfile);
		}
	}

	unsigned long packet_count = 0;
	while (running) {
		zmq_msg_t msg;
		ret = zmq_msg_init_size(&msg, 1024);
		if (ret < 0) {
			log_msg("ERROR", "Failed to init ZMQ message: %s", zmq_strerror(zmq_errno()));
			pthread_mutex_lock(&stats_lock);
			stats.errors++;
			pthread_mutex_unlock(&stats_lock);
			usleep(1000);
			continue;
		}

		/* Receive data */
		ret = zmq_msg_recv(&msg, subscriber, 0);
		if (ret < 0) {
			int err = zmq_errno();
			zmq_msg_close(&msg);

			if (err == ETERM || err == EINTR) {
				log_msg("INFO", "Capture task shutting down (errno=%d)", err);
				break;
			}

			log_msg("ERROR", "ZMQ receive error: %s (errno=%d)", zmq_strerror(err), err);
			pthread_mutex_lock(&stats_lock);
			stats.errors++;
			stats.last_error_time = time(NULL);
			pthread_mutex_unlock(&stats_lock);
			continue;
		}

		pthread_mutex_lock(&stats_lock);
		stats.packets_received++;
		pthread_mutex_unlock(&stats_lock);

		size_t datalen = zmq_msg_size(&msg);
		if (datalen < 5) {
			log_msg("WARN", "Packet too short: %zu bytes (expected >= 5)", datalen);
			while (zmq_msg_recv(&msg, subscriber, ZMQ_NOBLOCK) > 0)
				zmq_msg_close(&msg);
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

		/* Print header data (only if debug enabled to reduce spam) */
		if (debug) {
			csp_print(CSP_LL_TRACE, "Packet: Src %u, Dst %u, Dport %u, Sport %u, Pri %u, Flags 0x%02X, Size %" PRIu16 "\n",
				   packet->id.src, packet->id.dst, packet->id.dport,
				   packet->id.sport, packet->id.pri, packet->id.flags, packet->length);
		}

		packet_count++;
		if (packet_count % 1000 == 0) {
			log_msg("INFO", "Processed %lu packets (RX=%lu, TX=%lu, Errors=%lu)",
			        packet_count, stats.packets_received, stats.packets_sent, stats.errors);
		}

		/* Log packet in JSON format */
		log_packet_json(packet);

		zmq_msg_close(&msg);
	}

	log_msg("INFO", "Capture task exiting (processed %lu packets)", packet_count);
	free(packet);
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
				csp_print(CSP_LL_INFO,
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

	/* Install signal handlers */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);

	log_msg("INFO", "ZMQ Proxy starting (CSP v%d)", csp_conf.version);
	log_msg("INFO", "Subscribe: %s, Publish: %s", sub_str, pub_str);
	if (debug) {
		log_msg("INFO", "Debug mode enabled");
	}

	void * ctx = zmq_ctx_new();
	if (!ctx) {
		log_msg("FATAL", "Failed to create ZMQ context: %s", zmq_strerror(zmq_errno()));
		return 1;
	}
	global_ctx = ctx;

	/* Set ZMQ context options for better error handling */
	zmq_ctx_set(ctx, ZMQ_IO_THREADS, 2);
	zmq_ctx_set(ctx, ZMQ_MAX_SOCKETS, 1024);

	void * frontend = zmq_socket(ctx, ZMQ_XSUB);
	if (!frontend) {
		log_msg("FATAL", "Failed to create ZMQ_XSUB socket: %s", zmq_strerror(zmq_errno()));
		zmq_ctx_destroy(ctx);
		return 1;
	}

	/* Set socket options */
	int linger = 0;
	zmq_setsockopt(frontend, ZMQ_LINGER, &linger, sizeof(linger));

    ret = zmq_bind(frontend, sub_str);
	if (ret < 0) {
		log_msg("FATAL", "Failed to bind ZMQ_XSUB to %s: %s (errno=%d)",
		        sub_str, zmq_strerror(zmq_errno()), zmq_errno());
		zmq_close(frontend);
		zmq_ctx_destroy(ctx);
		return 1;
	}
	log_msg("INFO", "Frontend (XSUB) bound to %s", sub_str);

	void * backend = zmq_socket(ctx, ZMQ_XPUB);
	if (!backend) {
		log_msg("FATAL", "Failed to create ZMQ_XPUB socket: %s", zmq_strerror(zmq_errno()));
		zmq_close(frontend);
		zmq_ctx_destroy(ctx);
		return 1;
	}

	zmq_setsockopt(backend, ZMQ_LINGER, &linger, sizeof(linger));

    ret = zmq_bind(backend, pub_str);
	if (ret < 0) {
		log_msg("FATAL", "Failed to bind ZMQ_XPUB to %s: %s (errno=%d)",
		        pub_str, zmq_strerror(zmq_errno()), zmq_errno());
		zmq_close(backend);
		zmq_close(frontend);
		zmq_ctx_destroy(ctx);
		return 1;
	}
	log_msg("INFO", "Backend (XPUB) bound to %s", pub_str);

	pthread_t capworker;
	ret = pthread_create(&capworker, NULL, task_capture, ctx);
	if (ret != 0) {
		log_msg("FATAL", "Failed to create capture thread: %s", strerror(ret));
		zmq_close(backend);
		zmq_close(frontend);
		zmq_ctx_destroy(ctx);
		return 1;
	}
	log_msg("INFO", "Capture thread started");

	log_msg("INFO", "Starting ZMQ proxy loop...");

	/* Run the proxy - this blocks until context is terminated */
	ret = zmq_proxy(frontend, backend, NULL);

	if (ret < 0) {
		int err = zmq_errno();
		if (err == ETERM) {
			log_msg("INFO", "Proxy terminated gracefully");
		} else {
			log_msg("ERROR", "Proxy failed: %s (errno=%d)", zmq_strerror(err), err);
		}
	}

	log_msg("INFO", "Shutting down ZMQ proxy...");
	log_msg("INFO", "Final statistics: RX=%lu, TX=%lu, Errors=%lu",
	        stats.packets_received, stats.packets_sent, stats.errors);

	/* Cleanup */
	running = 0;
	pthread_join(capworker, NULL);

	zmq_close(backend);
	zmq_close(frontend);
	zmq_ctx_destroy(ctx);

	if (logfile) {
		fclose(logfile);
	}

	log_msg("INFO", "ZMQ proxy exited cleanly");
	return 0;
}
