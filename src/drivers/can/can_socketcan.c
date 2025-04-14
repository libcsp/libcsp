

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
		struct timeval timeout = {
			.tv_sec = 10,
		};
		int n = select(ctx->socket + 1, &input, NULL, NULL, &timeout);
		if (n == -1) {
			csp_print("CAN read error\n");
			continue;
		} else if (n == 0) {
			//printf("CAN idle\n");
			continue;
		}

		/* Read CAN or CAN FD frame */
		uint32_t can_id = 0;
		uint8_t * data = NULL;
		uint8_t data_len = 0;

		if (ctx->ifdata.enable_canfd) {
			struct canfd_frame frame;
			ssize_t nbytes = read(ctx->socket, &frame, sizeof(struct canfd_frame));
			if (nbytes < 0) {
				if (errno == EAGAIN || errno == EINTR) {
					/* This is acceptable, since something interrupted us, try again */
					continue;
				} else {
					csp_print("%s[%s]: read() failed, errno %d: %s\n", __func__, ctx->name, errno, strerror(errno));
					usleep(1 * 1E6);
					continue;
				}
			}

			if (nbytes != CANFD_MTU) {
				csp_print("%s[%s]: Incomplete CAN FD frame (%ld bytes)\n", __func__, ctx->name, nbytes);
				continue;
			}

			/* Drop frames with invalid size field */
			if (frame.len > CANFD_MAX_DLEN) {
				continue;
			}

			/* Drop frames with standard id (CSP uses extended) */
			if (!(frame.can_id & CAN_EFF_FLAG)) {
				continue;
			}

			/* Drop error and remote frames */
			if (frame.can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG)) {
				csp_print("%s[%s]: discarding ERR/RTR/SFF frame (FD)\n", __func__, ctx->name);
				continue;
			}

			/* Strip flags */
			can_id = frame.can_id & CAN_EFF_MASK;
			data = frame.data;
			data_len = frame.len;

		} else {
			struct can_frame frame;
			ssize_t nbytes = read(ctx->socket, &frame, sizeof(struct can_frame));
			if (nbytes < 0) {
				if (errno == EAGAIN || errno == EINTR) {
					/* This is acceptable, since something interrupted us, try again */
					continue;
				} else {
					csp_print("%s[%s]: read() failed, errno %d: %s\n", __func__, ctx->name, errno, strerror(errno));
					usleep(1 * 1E6);
					continue;
				}
			}

			if (nbytes != CAN_MTU) {
				csp_print("%s[%s]: Incomplete CAN frame (%ld bytes)\n", __func__, ctx->name, nbytes);
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
			can_id = frame.can_id & CAN_EFF_MASK;
			data = frame.data;
			data_len = frame.can_dlc;
		}

		/* Call RX callback */
		csp_can_rx(&ctx->iface, can_id, data, data_len, NULL);
	}

	/* We should never reach this point */
	pthread_exit(NULL);
}

static int csp_can_internal_tx_frame(can_context_t *ctx, const void *frame, size_t size) {
	uintptr_t pdata = (uintptr_t)frame;
	uintptr_t pend = pdata + size;
	uint32_t waiting_ms = 0;

	while (pdata < pend) {
		int written = write(ctx->socket, (void *)pdata, pend - pdata);
		if (written < 0) {
			if (errno == ENOBUFS || errno == EAGAIN || errno == EINTR) {
				usleep(5000);
				waiting_ms += 5;
				if (waiting_ms >= 1000) {
					csp_print("%s[%s]: write() timeout (>1000 ms)\n", __func__, ctx->name);
					return CSP_ERR_TX;
				}
			} else {
				csp_print("%s[%s]: write() failed, error %d - '%s'\n", __func__, ctx->name, errno, strerror(errno));
				return CSP_ERR_TX;
			}
		} else {
			waiting_ms = 0;
			pdata += written;
		}
	}

	return CSP_ERR_NONE;
}

static int csp_can_tx_frame(void *driver_data, uint32_t id, const uint8_t *data, uint8_t dlc) {
	can_context_t *ctx = driver_data;

	if (ctx->ifdata.enable_canfd) {
		if (dlc > CANFD_MAX_DLEN)
			return CSP_ERR_INVAL;

		struct canfd_frame frame = {
			.can_id = id | CAN_EFF_FLAG,
			.len = dlc
		};
		memcpy(frame.data, data, dlc);
		return csp_can_internal_tx_frame(ctx, &frame, sizeof(frame));
	} else {
		if (dlc > CAN_MAX_DLEN)
			return CSP_ERR_INVAL;

		struct can_frame frame = {
			.can_id = id | CAN_EFF_FLAG,
			.can_dlc = dlc
		};
		memcpy(frame.data, data, dlc);
		return csp_can_internal_tx_frame(ctx, &frame, sizeof(frame));
	}
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

static int csp_can_socketcan_open_internal(const char * device, const char * ifname, unsigned int node_id, int bitrate, bool promisc, bool enable_canfd, csp_iface_t ** return_iface) {
	if (ifname == NULL) {
		ifname = CSP_IF_CAN_DEFAULT_NAME;
	}

	csp_print("INIT %s (FD: %d): device: [%s], bitrate: %d, promisc: %d\n", ifname, enable_canfd, device, bitrate, promisc);

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
	ctx->ifdata.enable_canfd = enable_canfd;

	/* Create socket */
	if ((ctx->socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		csp_print("%s[%s]: socket() failed, error: %s\n", __func__, ctx->name, strerror(errno));
		socketcan_free(ctx);
		return CSP_ERR_INVAL;
	}

	if (enable_canfd) {
		if (setsockopt(ctx->socket, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &ctx->ifdata.enable_canfd, sizeof(ctx->ifdata.enable_canfd)) < 0) {
			csp_print("setsockopt(CAN_RAW_FD_FRAMES) failed: %s\n", strerror(errno));
			socketcan_free(ctx);
			return CSP_ERR_INVAL;
		}
	}

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
		socketcan_free(ctx);
		return CSP_ERR_INVAL;
	}

	/* Add interface to CSP */
	if (csp_can_add_interface(&ctx->iface) != CSP_ERR_NONE) {
		socketcan_free(ctx);
		return CSP_ERR_INVAL;
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

int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, unsigned int node_id, int bitrate, bool promisc, csp_iface_t ** return_iface) {
	return csp_can_socketcan_open_internal(device, ifname, node_id, bitrate, promisc, false, return_iface);
}

int csp_can_socketcan_open_fd_and_add_interface(const char * device, const char * ifname, unsigned int node_id, int bitrate, bool promisc, csp_iface_t ** return_iface) {
	return csp_can_socketcan_open_internal(device, ifname, node_id, bitrate, promisc, true, return_iface);
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
