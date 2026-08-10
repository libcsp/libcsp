

#include <csp/drivers/can_socketcan.h>

#include <stdio.h>
#include <string.h>
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
#include <csp/csp_id.h>

// CAN interface data, state, etc.
typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;
	csp_can_interface_data_t ifdata;
	pthread_t rx_thread;
	int socket;
	bool fd;
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

		/* Read CAN frame, classic (CAN_MTU) or CAN FD (CANFD_MTU) sized */
		struct canfd_frame frame;
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

		if ((nbytes != CAN_MTU) && (nbytes != CANFD_MTU)) {
			csp_print("%s[%s]: Read incomplete CAN frame, size: %d bytes\n", __func__, ctx->name, nbytes);
			continue;
		}

		/* Drop frames with invalid size field (len aliases can_dlc).
		 * Linux: CAN_MAX_DLEN (8) and CANFD_MAX_DLEN (64) are fixed constants */
		if (frame.len > ((nbytes == CANFD_MTU) ? CANFD_MAX_DLEN : CAN_MAX_DLEN)) {
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

		/* Call RX callback */
		csp_can_rx(&ctx->iface, frame.can_id, frame.data, frame.len, 0, NULL);
	}

	/* We should never reach this point */
	pthread_exit(NULL);
}

static int csp_can_tx_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t data_size, const csp_packet_t *packet) {
	(void)packet;
	can_context_t * ctx = driver_data;

	/* struct canfd_frame is a layout compatible superset of struct can_frame */
	struct canfd_frame frame = {.can_id = id | CAN_EFF_FLAG,
								.len = data_size};
	size_t frame_size;

	/* Linux: CAN_MAX_DLEN (8) and CANFD_MAX_DLEN (64) are fixed constants,
	 * unlike Zephyr where CAN_MAX_DLEN depends on the build */
	if (ctx->fd) {
		if (data_size > CANFD_MAX_DLEN) {
			return CSP_ERR_INVAL;
		}
		/* Bit Rate Switch: data phase at the CAN FD data bitrate */
		frame.flags = CANFD_BRS;
		frame_size = CANFD_MTU;
	} else {
		if (data_size > CAN_MAX_DLEN) {
			return CSP_ERR_INVAL;
		}
		frame_size = CAN_MTU;
	}
	memcpy(frame.data, data, data_size);

	uint32_t waiting_ms = 0;
	uintptr_t pdata = (uintptr_t)&frame;
	uintptr_t pend = ((uintptr_t)&frame + frame_size);
	size_t length = frame_size;

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


static int csp_can_socketcan_set_promisc(const bool promisc, can_context_t * ctx) {

	struct can_filter filter[3] = { {
		.can_id = CFP_MAKE_DST(ctx->iface.addr),
		.can_mask = 0x0000, /* receive anything */
	} };

	if (ctx->socket == 0) {
		return CSP_ERR_INVAL;
	}

	int num_filters = 1;
	if (!promisc) {
		if (csp_conf.version == 1) {
			num_filters = 1;
			filter[0].can_id = CFP_MAKE_DST(ctx->iface.addr);
			filter[0].can_mask = CFP_MAKE_DST((1 << CFP_HOST_SIZE) - 1);
		} else {
			num_filters = 3;
			filter[0].can_id = ctx->iface.addr << CFP2_DST_OFFSET;
			filter[0].can_mask = CFP2_DST_MASK << CFP2_DST_OFFSET;
			filter[1].can_id = ((1 << (csp_id_get_host_bits() - ctx->iface.netmask)) - 1) << CFP2_DST_OFFSET;
			filter[1].can_mask = CFP2_DST_MASK << CFP2_DST_OFFSET;
			filter[2].can_id = 0x3FFF << CFP2_DST_OFFSET;
			filter[2].can_mask = CFP2_DST_MASK << CFP2_DST_OFFSET;
		}
	}

	if (setsockopt(ctx->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, num_filters * sizeof(struct can_filter)) < 0) {
		csp_print("%s: setsockopt() failed, error: %s\n", __func__, strerror(errno));
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}

static int csp_can_socketcan_add_alias(void * driver_data, uint16_t addr) {

	if (csp_conf.version == 1) {
		return -1;
	}

	can_context_t * ctx = driver_data;

	struct can_filter filter[10];
	socklen_t len = sizeof(filter);

	getsockopt(ctx->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, &len);

	/* Current implementation has a defined maximum of filters available */
	if (len == sizeof(filter)) {
		return -2;
	}

	/* If only 1 filter exist for CSP v2, interface is promisc */
	if (len == sizeof(struct can_filter)) {
		return 0;
	}

	/* Add filter for specific additional receive address */
	filter[len/sizeof(struct can_filter)].can_id = addr << CFP2_DST_OFFSET;
	filter[len/sizeof(struct can_filter)].can_mask = CFP2_DST_MASK << CFP2_DST_OFFSET;;

	if (setsockopt(ctx->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, len + sizeof(struct can_filter)) < 0) {
		return -2;
	}

	return 0;
}

int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, unsigned int node_id, int bitrate, bool fd, bool promisc, csp_iface_t ** return_iface) {
	if (ifname == NULL) {
		ifname = fd ? CSP_IF_CANFD_DEFAULT_NAME : CSP_IF_CAN_DEFAULT_NAME;
	}

	/* The CAN FD profile is fixed, see csp_if_can.h - bitrate is not applicable */
	if (fd && (bitrate != 0)) {
		csp_print("%s[%s]: CAN FD runs a fixed profile (%d/%d), bitrate must be 0\n",
				  __func__, ifname, CSP_CANFD_BITRATE, CSP_CANFD_DATA_BITRATE);
		return CSP_ERR_INVAL;
	}

	if (fd) {
		csp_print("INIT %s: device: [%s], bitrate: %d/%d, promisc: %d, fd: 1\n", ifname, device, CSP_CANFD_BITRATE, CSP_CANFD_DATA_BITRATE, promisc);
	} else {
		csp_print("INIT %s: device: [%s], bitrate: %d, promisc: %d, fd: 0\n", ifname, device, bitrate, promisc);
	}

	/* Set interface up - this may require increased OS privileges */
	if (fd) {
#if (CSP_HAVE_LIBSOCKETCAN_CANFD)
		/* Configure the fixed CAN FD profile, see csp_if_can.h (requires libsocketcan newer than v0.0.12) */
		struct can_bittiming bt = {.bitrate = CSP_CANFD_BITRATE};
		struct can_bittiming dbt = {.bitrate = CSP_CANFD_DATA_BITRATE};
		can_do_stop(device);
		can_set_canfd_bittiming(device, &bt, &dbt);
		can_set_restart_ms(device, 100);
		can_do_start(device);
#endif
	} else if (bitrate > 0) {
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
	ctx->fd = fd;

	strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
	ctx->iface.name = ctx->name;
	ctx->iface.addr = node_id;
	ctx->iface.interface_data = &ctx->ifdata;
	ctx->iface.driver_data = ctx;
	ctx->ifdata.tx_func = csp_can_tx_frame;
	ctx->ifdata.max_frame_size = fd ? CSP_CANFD_FRAME_SIZE : CSP_CAN_FRAME_SIZE;
	ctx->iface.add_alias = csp_can_socketcan_add_alias;
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
	int can_ifindex = ifr.ifr_ifindex;

	if (fd) {
		/* Verify the link is FD enabled - configuration may have failed or be
		 * unsupported by the installed libsocketcan */
		if ((ioctl(ctx->socket, SIOCGIFMTU, &ifr) < 0) || (ifr.ifr_mtu != CANFD_MTU)) {
			csp_print("%s[%s]: device [%s] is not CAN FD enabled, configure the link with e.g.:\n"
					  "  ip link set %s up type can bitrate 1000000 dbitrate 4000000 fd on restart-ms 100\n",
					  __func__, ctx->name, device, device);
			socketcan_free(ctx);
			return CSP_ERR_INVAL;
		}

		/* Accept and transmit CAN FD frames on this socket */
		int enable = 1;
		if (setsockopt(ctx->socket, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable, sizeof(enable)) < 0) {
			csp_print("%s[%s]: setsockopt(CAN_RAW_FD_FRAMES) failed, error: %s\n", __func__, ctx->name, strerror(errno));
			socketcan_free(ctx);
			return CSP_ERR_INVAL;
		}
	}

	fcntl(ctx->socket, F_SETFL, O_NONBLOCK);

	struct sockaddr_can addr;
	memset(&addr, 0, sizeof(addr));
	/* Bind the socket to CAN interface */
	addr.can_family = AF_CAN;
	addr.can_ifindex = can_ifindex;
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
	int res = csp_can_add_interface(&ctx->iface);
	if (res != CSP_ERR_NONE) {
		csp_print("%s[%s]: csp_can_add_interface() failed, error: %d\n", __func__, ctx->name, res);
		socketcan_free(ctx);
		return res;
	}

	/* Create receive thread */
	if (pthread_create(&ctx->rx_thread, NULL, socketcan_rx_thread, ctx) != 0) {
		csp_print("%s[%s]: pthread_create() failed, error: %s\n", __func__, ctx->name, strerror(errno));
		(void)csp_can_remove_interface(&ctx->iface);
		socketcan_free(ctx);
		return CSP_ERR_NOMEM;
	}

	if (return_iface) {
		*return_iface = &ctx->iface;
	}

	return CSP_ERR_NONE;
}

csp_iface_t * csp_can_socketcan_init(const char * device, unsigned int node_id, int bitrate, bool promisc) {
	csp_iface_t * return_iface;
	int res = csp_can_socketcan_open_and_add_interface(device, CSP_IF_CAN_DEFAULT_NAME, node_id, bitrate, false, promisc, &return_iface);
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
