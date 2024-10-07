#include <csp/interfaces/csp_if_zmqhub.h>

#if (CSP_HAVE_LIBZMQ)

#include <zmq.h>
#include <assert.h>
#include <stdlib.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <pthread.h>

#include <csp/csp_id.h>

/* ZMQ driver & interface */
typedef struct {
	pthread_t rx_thread;
	void * context;
	void * publisher;
	void * subscriber;
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;
} zmq_driver_t;


#define CURVE_KEYLEN 41

/* Linux is fast, so we keep it simple by having a single lock */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Interface transmit function
 * @param packet Packet to transmit
 * @return 1 if packet was successfully transmitted, 0 on error
 */
int csp_zmqhub_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	zmq_driver_t * drv = iface->driver_data;

	csp_id_prepend(packet);

	/** 
	 * While a ZMQ context is thread safe, sockets are NOT threadsafe, so by sharing drv->publisher, we 
	 * need to have a lock around any calls that uses that */
	pthread_mutex_lock(&lock);
	int result = zmq_send(drv->publisher, packet->frame_begin, packet->frame_length, 0);
	pthread_mutex_unlock(&lock);

	if (result < 0) {
		csp_print("ZMQ send error: %u %s\n", result, zmq_strerror(zmq_errno()));
	}

	csp_buffer_free(packet);

	return CSP_ERR_NONE;
}

void * csp_zmqhub_task(void * param) {

	zmq_driver_t * drv = param;
	csp_packet_t * packet;
	const uint32_t HEADER_SIZE = (csp_conf.version == 2) ? 6 : 4;

	while (1) {
		int ret;
		(void)ret; /* Silence unused variable warning (promoted to an error if -Werr) issued when building with NDEBUG (release with asserts turned off) */
		zmq_msg_t msg;

		ret = zmq_msg_init_size(&msg, sizeof(packet->data) + HEADER_SIZE);
		assert(ret == 0);

		// Receive data
		if (zmq_msg_recv(&msg, drv->subscriber, 0) < 0) {
			csp_print("ZMQ RX err %s: %s\n", drv->iface.name, zmq_strerror(zmq_errno()));
			continue;
		}

		unsigned int datalen = zmq_msg_size(&msg);
		if (datalen < HEADER_SIZE) {
			csp_print("ZMQ RX %s: Too short datalen: %u - expected min %u bytes\n", drv->iface.name, datalen, HEADER_SIZE);
			zmq_msg_close(&msg);
			continue;
		}

		// Create new csp packet
		packet = csp_buffer_get(0);
		if (packet == NULL) {
			csp_print("RX %s: Failed to get csp_buffer(%u)\n", drv->iface.name, datalen);
			zmq_msg_close(&msg);
			continue;
		}

		// Copy the data from zmq to csp
		const uint8_t * rx_data = zmq_msg_data(&msg);

		csp_id_setup_rx(packet);

		memcpy(packet->frame_begin, rx_data, datalen);
		packet->frame_length = datalen;

		/* Parse the frame and strip the ID field */
		if (csp_id_strip(packet) != 0) {
			drv->iface.rx_error++;
			csp_buffer_free(packet);
		    zmq_msg_close(&msg);
			continue;
		}

		// Route packet
		csp_qfifo_write(packet, &drv->iface, NULL);

		zmq_msg_close(&msg);
	}

	return NULL;
}

int csp_zmqhub_make_endpoint(const char * host, uint16_t port, char * buf, size_t buf_size) {
	int res = snprintf(buf, buf_size, "tcp://%s:%u", host, port);
	if ((res < 0) || (res >= (int)buf_size)) {
		buf[0] = 0;
		return CSP_ERR_NOMEM;
	}
	return CSP_ERR_NONE;
}

int csp_zmqhub_init(uint16_t addr,
					const char * host,
					uint32_t flags,
					csp_iface_t ** return_interface) {

	char pub[100];
	csp_zmqhub_make_endpoint(host, CSP_ZMQPROXY_SUBSCRIBE_PORT, pub, sizeof(pub));

	char sub[100];
	csp_zmqhub_make_endpoint(host, CSP_ZMQPROXY_PUBLISH_PORT, sub, sizeof(sub));

	return csp_zmqhub_init_w_endpoints(addr, pub, sub, flags, return_interface);
}

int csp_zmqhub_init_w_endpoints(uint16_t addr,
								const char * publisher_endpoint,
								const char * subscriber_endpoint,
								uint32_t flags,
								csp_iface_t ** return_interface) {

	uint16_t * rxfilter = NULL;
	unsigned int rxfilter_count = 0;

	return csp_zmqhub_init_w_name_endpoints_rxfilter(NULL, addr,
													 rxfilter, rxfilter_count,
													 publisher_endpoint,
													 subscriber_endpoint,
													 flags,
													 return_interface);
}

int csp_zmqhub_init_w_name_endpoints_rxfilter(const char * ifname, uint16_t addr,
											  const uint16_t rxfilter[], unsigned int rxfilter_count,
											  const char * publish_endpoint,
											  const char * subscribe_endpoint,
											  uint32_t flags,
											  csp_iface_t ** return_interface) {

	int ret;
	pthread_attr_t attributes;
	zmq_driver_t * drv = calloc(1, sizeof(*drv));
	assert(drv != NULL);

	if (ifname == NULL) {
		ifname = CSP_ZMQHUB_IF_NAME;
	}

	strncpy(drv->name, ifname, sizeof(drv->name) - 1);
	drv->iface.name = drv->name;
	drv->iface.driver_data = drv;
	drv->iface.nexthop = csp_zmqhub_tx;
	drv->iface.addr = addr;

	drv->context = zmq_ctx_new();
	assert(drv->context != NULL);

	//csp_print("INIT %s: pub(tx): [%s], sub(rx): [%s], rx filters: %u", drv->iface.name, publish_endpoint, subscribe_endpoint, rxfilter_count);

	/* Publisher (TX) */
	drv->publisher = zmq_socket(drv->context, ZMQ_PUB);
	assert(drv->publisher != NULL);

	/* Subscriber (RX) */
	drv->subscriber = zmq_socket(drv->context, ZMQ_SUB);
	assert(drv->subscriber != NULL);

	// subscribe to all packets - no filter
	ret = zmq_setsockopt(drv->subscriber, ZMQ_SUBSCRIBE, NULL, 0);
	assert(ret == 0);

	/* Connect to server */
	ret = zmq_connect(drv->publisher, publish_endpoint);
	assert(ret == 0);
	zmq_connect(drv->subscriber, subscribe_endpoint);
	assert(ret == 0);

	/* Start RX thread */
	ret = pthread_attr_init(&attributes);
	assert(ret == 0);
	ret = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	assert(ret == 0);
	ret = pthread_create(&drv->rx_thread, &attributes, csp_zmqhub_task, drv);
	assert(ret == 0);

	/* Register interface */
	csp_iflist_add(&drv->iface);

	if (return_interface) {
		*return_interface = &drv->iface;
	}

	return CSP_ERR_NONE;
}

int csp_zmqhub_init_filter2(const char * ifname, const char * host, uint16_t addr, uint16_t netmask, int promisc, csp_iface_t ** return_interface, char * sec_key, uint16_t subport, uint16_t pubport) {
	
	char pub[100];
	csp_zmqhub_make_endpoint(host, subport, pub, sizeof(pub));

	char sub[100];
	csp_zmqhub_make_endpoint(host, pubport, sub, sizeof(sub));

	int ret;
	pthread_attr_t attributes;
	zmq_driver_t * drv = calloc(1, sizeof(*drv));
	assert(drv != NULL);

	if (ifname == NULL) {
		ifname = CSP_ZMQHUB_IF_NAME;
	}

	strncpy(drv->name, ifname, sizeof(drv->name) - 1);
	drv->iface.name = drv->name;
	drv->iface.driver_data = drv;
	drv->iface.nexthop = csp_zmqhub_tx;

	drv->context = zmq_ctx_new();
	assert(drv->context != NULL);

	csp_print("  ZMQ init %s: addr: %u, pub(tx): [%s], sub(rx): [%s]\n", drv->iface.name, addr, pub, sub);

	/* Publisher (TX) */
	drv->publisher = zmq_socket(drv->context, ZMQ_PUB);
	assert(drv->publisher != NULL);

	/* Subscriber (RX) */
	drv->subscriber = zmq_socket(drv->context, ZMQ_SUB);
	assert(drv->subscriber != NULL);

	/* If shared secret key provided */
	if (sec_key) {
		char pub_key[41];

		zmq_curve_public(pub_key, sec_key);
		/* Publisher (TX) */
		zmq_setsockopt(drv->publisher, ZMQ_CURVE_SERVERKEY, pub_key, CURVE_KEYLEN);
		zmq_setsockopt(drv->publisher, ZMQ_CURVE_PUBLICKEY, pub_key, CURVE_KEYLEN);
		zmq_setsockopt(drv->publisher, ZMQ_CURVE_SECRETKEY, sec_key, CURVE_KEYLEN);
		/* Subscriber (RX) */
		zmq_setsockopt(drv->subscriber, ZMQ_CURVE_SERVERKEY, pub_key, CURVE_KEYLEN);
		zmq_setsockopt(drv->subscriber, ZMQ_CURVE_PUBLICKEY, pub_key, CURVE_KEYLEN);
		zmq_setsockopt(drv->subscriber, ZMQ_CURVE_SECRETKEY, sec_key, CURVE_KEYLEN);
	}
	int keep_alive = 1;
	/* Time in seconds a connection must be idle before keep-alive packet send*/
	int idle = 900;
	/* Maximum number of keep-alive probes to send without ack before connection closed */
	int cnt = 2;
	/* Interval in seconds between each keep-alive probe */
	int intvl = 900;
	/* Publisher (TX) */
	zmq_setsockopt(drv->publisher, ZMQ_TCP_KEEPALIVE, &keep_alive, sizeof(keep_alive));
	zmq_setsockopt(drv->publisher, ZMQ_TCP_KEEPALIVE_IDLE, &idle, sizeof(idle));
	zmq_setsockopt(drv->publisher, ZMQ_TCP_KEEPALIVE_CNT, &cnt, sizeof(cnt));
	zmq_setsockopt(drv->publisher, ZMQ_TCP_KEEPALIVE_INTVL, &intvl, sizeof(intvl));
	/* Subscriber (RX) */
	zmq_setsockopt(drv->subscriber, ZMQ_TCP_KEEPALIVE, &keep_alive, sizeof(keep_alive));
	zmq_setsockopt(drv->subscriber, ZMQ_TCP_KEEPALIVE_IDLE, &idle, sizeof(idle));
	zmq_setsockopt(drv->subscriber, ZMQ_TCP_KEEPALIVE_CNT, &cnt, sizeof(cnt));
	zmq_setsockopt(drv->subscriber, ZMQ_TCP_KEEPALIVE_INTVL, &intvl, sizeof(intvl));

	/* Generate filters */
	uint16_t hostmask = (1 << (csp_id_get_host_bits() - netmask)) - 1;
	
	/* Connect to server */
	ret = zmq_connect(drv->publisher, pub);
	assert(ret == 0);
	ret = zmq_connect(drv->subscriber, sub);
	assert(ret == 0);


	if (promisc) {

		// subscribe to all packets - no filter
		ret = zmq_setsockopt(drv->subscriber, ZMQ_SUBSCRIBE, NULL, 0);
		assert(ret == 0);

	} else {

		/* This needs to be static, because ZMQ does not copy the filter value to the
		 * outgoing packet for each setsockopt call */
		static uint16_t filt[4][3];

		for (int i = 0; i < 4; i++) {
			//int i = CSP_PRIO_NORM;
			filt[i][0] = __builtin_bswap16((i << 14) | addr);
			filt[i][1] = __builtin_bswap16((i << 14) | addr | hostmask);
			filt[i][2] = __builtin_bswap16((i << 14) | 16383);
			ret = zmq_setsockopt(drv->subscriber, ZMQ_SUBSCRIBE, &filt[i][0], 2);
			ret = zmq_setsockopt(drv->subscriber, ZMQ_SUBSCRIBE, &filt[i][1], 2);
			ret = zmq_setsockopt(drv->subscriber, ZMQ_SUBSCRIBE, &filt[i][2], 2);
		}

	} 


	/* Start RX thread */
	ret = pthread_attr_init(&attributes);
	assert(ret == 0);
	ret = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	assert(ret == 0);
	ret = pthread_create(&drv->rx_thread, &attributes, csp_zmqhub_task, drv);
	assert(ret == 0);

	/* Register interface */
	csp_iflist_add(&drv->iface);

	if (return_interface) {
		*return_interface = &drv->iface;
	}

	return CSP_ERR_NONE;
}


#endif  // CSP_HAVE_LIBZMQ
