

#include <csp/drivers/can_socketcan.h>

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <csp/csp_debug.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/can/raw.h>
#include <libsocketcan.h>

#include <csp/csp.h>

// CAN interface data, state, etc.
typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;
	csp_can_interface_data_t ifdata;
	pthread_t rx_thread;
	int socket;
} can_context_t;

static void socketcan_free(can_context_t * ctx) {

	if (ctx) {
		if (ctx->socket >= 0) {
			close(ctx->socket);
		}
		free(ctx);
	}
}

static void * socketcan_rx_thread(void * arg) {
	can_context_t * ctx = arg;

	while (1) {

		/* Use select for non blocking reads */
		fd_set input;
		FD_ZERO(&input);
		FD_SET(ctx->socket, &input);
		struct timeval timeout;
		timeout.tv_sec = 10;
		int n = select(ctx->socket + 1, &input, NULL, NULL, &timeout);
		if (n == -1) {
			csp_print("CAN read error\n");
			continue;
		} else if (n == 0) {
			//printf("CAN idle\n");
			continue;
		}

		/* Read CAN frame */
		struct can_frame frame;
		int nbytes = read(ctx->socket, &frame, sizeof(frame)); 
		if (nbytes < 0) {
			if (errno == EAGAIN || errno == EINTR) {
				/* This is acceptable, since something interrupted us, try again */
				continue;
			} else {
				csp_print("%s[%s]: read() failed, errno %d: %s\n", __func__, ctx->name, errno, strerror(errno));
				usleep(1*1E6);
				continue;
			}
		}

		if (nbytes != sizeof(frame)) {
			csp_print("%s[%s]: Read incomplete CAN frame, size: %d, expected: %u bytes\n", __func__, ctx->name, nbytes, (unsigned int)sizeof(frame));
			continue;
		}

		/* Drop frames with invalid size field */
		if (frame.can_dlc > CAN_MAX_DLEN) {
			continue;
		}

		/* Drop frames with standard id (CSP uses extended) */
		if (!(frame.can_id & CAN_EFF_FLAG)) {
			continue;
		}

		/* Drop error and remote frames */
		if (frame.can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG)) {
			csp_print("%s[%s]: discarding ERR/RTR/SFF frame\n", __func__, ctx->name);
			continue;
		}

		/* Strip flags */
		frame.can_id &= CAN_EFF_MASK;

		/* Call RX callbacsp_can_rx_frameck */
		csp_can_rx(&ctx->iface, frame.can_id, frame.data, frame.can_dlc, NULL);
	}

	/* We should never reach this point */
	pthread_exit(NULL);
}

static int csp_can_tx_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc) {
	if (dlc > CAN_MAX_DLEN) {
		return CSP_ERR_INVAL;
	}

	struct can_frame frame = {.can_id = id | CAN_EFF_FLAG,
							  .can_dlc = dlc};
	memcpy(frame.data, data, dlc);

	uint32_t waiting_ms = 0;
	can_context_t * ctx = driver_data;
	uintptr_t pdata = (uintptr_t)&frame;
	uintptr_t pend = ((uintptr_t)&frame + sizeof(frame));
	size_t length = sizeof(frame);

	while (pdata < pend) {
		int written;
		
		written = write(ctx->socket, (void *)pdata, length);
		if (written < 0) {
			if (errno == ENOBUFS) {
				/* If no space available, wait for 5 ms and try again */
				usleep(5000);
				waiting_ms += 5;
			} else if(errno == EAGAIN || errno == EINTR) {
				/* Acceptable, since something interrupted us, try again */
				waiting_ms += 5;
			} else {
				csp_print("%s[%s]: write() failed, encountered an error during write(). %d - '%s'\n", __func__, ctx->name, errno, strerror(errno));
				return CSP_ERR_TX;
			}

			if (waiting_ms >= 1000) {
				/* We finally got tired of waiting, give up */
				csp_print("%s[%s]: write() failed, we have been waiting for CAN buffers for too long (>1000 ms)\n", __func__, ctx->name);
				return CSP_ERR_TX;
			}
		} else {
			waiting_ms = 0;
			pdata += written;
			length -= written;
		}
	}

	return CSP_ERR_NONE;
}


int csp_can_socketcan_set_promisc(const bool promisc, can_context_t * ctx) {
	struct can_filter filter = {
		.can_id = CFP_MAKE_DST(ctx->iface.addr),
		.can_mask = 0x0000, /* receive anything */
	};

	if (ctx->socket == 0) {
		return CSP_ERR_INVAL;
	}

	if (!promisc) {
		if (csp_conf.version == 1) {
			filter.can_id = CFP_MAKE_DST(ctx->iface.addr);
			filter.can_mask = CFP_MAKE_DST((1 << CFP_HOST_SIZE) - 1);
		} else {
			filter.can_id = ctx->iface.addr << CFP2_DST_OFFSET;
			filter.can_mask = CFP2_DST_MASK << CFP2_DST_OFFSET;
		}
	}

	if (setsockopt(ctx->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
		csp_print("%s: setsockopt() failed, error: %s\n", __func__, strerror(errno));
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}


int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, unsigned int node_id, int bitrate, bool promisc, csp_iface_t ** return_iface) {
	if (ifname == NULL) {
		ifname = CSP_IF_CAN_DEFAULT_NAME;
	}

	csp_print("INIT %s: device: [%s], bitrate: %d, promisc: %d\n", ifname, device, bitrate, promisc);

	/* Set interface up - this may require increased OS privileges */
	if (bitrate > 0) {
		can_do_stop(device);
		can_set_bitrate(device, bitrate);
		can_set_restart_ms(device, 100);
		can_do_start(device);
	}

	can_context_t * ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		return CSP_ERR_NOMEM;
	}
	ctx->socket = -1;

	strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
	ctx->iface.name = ctx->name;
	ctx->iface.addr = node_id;
	ctx->iface.interface_data = &ctx->ifdata;
	ctx->iface.driver_data = ctx;
	ctx->ifdata.tx_func = csp_can_tx_frame;
	ctx->ifdata.pbufs = NULL;

	/* Create socket */
	if ((ctx->socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		csp_print("%s[%s]: socket() failed, error: %s\n", __func__, ctx->name, strerror(errno));
		socketcan_free(ctx);
		return CSP_ERR_INVAL;
	}

	/* Locate interface */
	struct ifreq ifr;
	strncpy(ifr.ifr_name, device, IFNAMSIZ - 1);
	if (ioctl(ctx->socket, SIOCGIFINDEX, &ifr) < 0) {
		csp_print("%s[%s]: device: [%s], ioctl() failed, error: %s\n", __func__, ctx->name, device, strerror(errno));
		socketcan_free(ctx);
		return CSP_ERR_INVAL;
	}

	fcntl(ctx->socket, F_SETFL, O_NONBLOCK);

	struct sockaddr_can addr;
	memset(&addr, 0, sizeof(addr));
	/* Bind the socket to CAN interface */
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(ctx->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		csp_print("%s[%s]: bind() failed, error: %s\n", __func__, ctx->name, strerror(errno));
		socketcan_free(ctx);
		return CSP_ERR_INVAL;
	}

	/* Set filter mode */
	if (csp_can_socketcan_set_promisc(promisc, ctx) != CSP_ERR_NONE) {
		csp_print("%s[%s]: csp_can_socketcan_set_promisc() failed, error: %s\n", __func__, ctx->name, strerror(errno));
		return CSP_ERR_INVAL;
	}

	/* Add interface to CSP */
	int res = csp_can_add_interface(&ctx->iface);
	if (res != CSP_ERR_NONE) {
		csp_print("%s[%s]: csp_can_add_interface() failed, error: %d\n", __func__, ctx->name, res);
		socketcan_free(ctx);
		return res;
	}

	/* Create receive thread */
	if (pthread_create(&ctx->rx_thread, NULL, socketcan_rx_thread, ctx) != 0) {
		csp_print("%s[%s]: pthread_create() failed, error: %s\n", __func__, ctx->name, strerror(errno));
		// socketcan_free(ctx); // we already added it to CSP (no way to remove it)
		return CSP_ERR_NOMEM;
	}

	if (return_iface) {
		*return_iface = &ctx->iface;
	}

	return CSP_ERR_NONE;
}

csp_iface_t * csp_can_socketcan_init(const char * device, unsigned int node_id, int bitrate, bool promisc) {
	csp_iface_t * return_iface;
	int res = csp_can_socketcan_open_and_add_interface(device, CSP_IF_CAN_DEFAULT_NAME, node_id, bitrate, promisc, &return_iface);
	return (res == CSP_ERR_NONE) ? return_iface : NULL;
}

int csp_can_socketcan_stop(csp_iface_t * iface) {
	can_context_t * ctx = iface->driver_data;

	int error = pthread_cancel(ctx->rx_thread);
	if (error != 0) {
		csp_print("%s[%s]: pthread_cancel() failed, error: %s\n", __func__, ctx->name, strerror(errno));
		return CSP_ERR_DRIVER;
	}
	error = pthread_join(ctx->rx_thread, NULL);
	if (error != 0) {
		csp_print("%s[%s]: pthread_join() failed, error: %s\n", __func__, ctx->name, strerror(errno));
		return CSP_ERR_DRIVER;
	}
	socketcan_free(ctx);
	return CSP_ERR_NONE;
}
