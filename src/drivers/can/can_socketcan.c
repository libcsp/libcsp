/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <csp/drivers/can_socketcan.h>

#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can/raw.h>
#if (CSP_HAVE_LIBSOCKETCAN)
#include <libsocketcan.h>
#endif

#include <csp/csp.h>
#include <csp/arch/csp_thread.h>

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

static void * socketcan_rx_thread(void * arg)
{
	can_context_t * ctx = arg;

	while (1) {
		/* Read CAN frame */
		struct can_frame frame;
		int nbytes = read(ctx->socket, &frame, sizeof(frame));
		if (nbytes < 0) {
			csp_log_error("%s[%s]: read() failed, errno %d: %s", __FUNCTION__, ctx->name, errno, strerror(errno));
			continue;
		}

		if (nbytes != sizeof(frame)) {
			csp_log_warn("%s[%s]: Read incomplete CAN frame, size: %d, expected: %u bytes", __FUNCTION__, ctx->name, nbytes, (unsigned int) sizeof(frame));
			continue;
		}

		/* Drop frames with standard id (CSP uses extended) */
		if (!(frame.can_id & CAN_EFF_FLAG)) {
			continue;
		}

		/* Drop error and remote frames */
		if (frame.can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG)) {
			csp_log_warn("%s[%s]: discarding ERR/RTR/SFF frame", __FUNCTION__, ctx->name);
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


static int csp_can_tx_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc)
{
	if (dlc > 8) {
		return CSP_ERR_INVAL;
	}

	struct can_frame frame = {.can_id = id | CAN_EFF_FLAG,
                                  .can_dlc = dlc};
        memcpy(frame.data, data, dlc);

	uint32_t elapsed_ms = 0;
        can_context_t * ctx = driver_data;
	while (write(ctx->socket, &frame, sizeof(frame)) != sizeof(frame)) {
		if ((errno != ENOBUFS) || (elapsed_ms >= 1000)) {
			csp_log_warn("%s[%s]: write() failed, errno %d: %s", __FUNCTION__, ctx->name, errno, strerror(errno));
			return CSP_ERR_TX;
		}
		csp_sleep_ms(5);
		elapsed_ms += 5;
	}

	return CSP_ERR_NONE;
}

int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, int bitrate, bool promisc, csp_iface_t ** return_iface)
{
	if (ifname == NULL) {
		ifname = CSP_IF_CAN_DEFAULT_NAME;
	}

	csp_log_info("INIT %s: device: [%s], bitrate: %d, promisc: %d",
			ifname, device, bitrate, promisc);

#if (CSP_HAVE_LIBSOCKETCAN)
	/* Set interface up - this may require increased OS privileges */
	if (bitrate > 0) {
		can_do_stop(device);
		can_set_bitrate(device, bitrate);
		can_set_restart_ms(device, 100);
		can_do_start(device);
	}
#endif

	can_context_t * ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		return CSP_ERR_NOMEM;
	}
	ctx->socket = -1;

	strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
	ctx->iface.name = ctx->name;
	ctx->iface.interface_data = &ctx->ifdata;
	ctx->iface.driver_data = ctx;
	ctx->ifdata.tx_func = csp_can_tx_frame;

	/* Create socket */
	if ((ctx->socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		csp_log_error("%s[%s]: socket() failed, error: %s", __FUNCTION__, ctx->name, strerror(errno));
		socketcan_free(ctx);
		return CSP_ERR_INVAL;
	}

	/* Locate interface */
	struct ifreq ifr;
	strncpy(ifr.ifr_name, device, IFNAMSIZ - 1);
	if (ioctl(ctx->socket, SIOCGIFINDEX, &ifr) < 0) {
		csp_log_error("%s[%s]: device: [%s], ioctl() failed, error: %s", __FUNCTION__, ctx->name, device, strerror(errno));
		socketcan_free(ctx);
		return CSP_ERR_INVAL;
	}
	struct sockaddr_can addr;
	memset(&addr, 0, sizeof(addr));
	/* Bind the socket to CAN interface */
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(ctx->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		csp_log_error("%s[%s]: bind() failed, error: %s", __FUNCTION__, ctx->name, strerror(errno));
		socketcan_free(ctx);
		return CSP_ERR_INVAL;
	}

	/* Set filter mode */
	if (promisc == false) {

		struct can_filter filter = {.can_id = CFP_MAKE_DST(csp_get_address()),
						.can_mask = CFP_MAKE_DST((1 << CFP_HOST_SIZE) - 1)};

		if (setsockopt(ctx->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
			csp_log_error("%s[%s]: setsockopt() failed, error: %s", __FUNCTION__, ctx->name, strerror(errno));
			socketcan_free(ctx);
			return CSP_ERR_INVAL;
		}
	}

	/* Add interface to CSP */
        int res = csp_can_add_interface(&ctx->iface);
	if (res != CSP_ERR_NONE) {
		csp_log_error("%s[%s]: csp_can_add_interface() failed, error: %d", __FUNCTION__, ctx->name, res);
		socketcan_free(ctx);
		return res;
	}

	/* Create receive thread */
	if (pthread_create(&ctx->rx_thread, NULL, socketcan_rx_thread, ctx) != 0) {
		csp_log_error("%s[%s]: pthread_create() failed, error: %s", __FUNCTION__, ctx->name, strerror(errno));
		//socketcan_free(ctx); // we already added it to CSP (no way to remove it)
		return CSP_ERR_NOMEM;
	}

	if (return_iface) {
		*return_iface = &ctx->iface;
	}

	return CSP_ERR_NONE;
}

csp_iface_t * csp_can_socketcan_init(const char * device, int bitrate, bool promisc)
{
	csp_iface_t * return_iface;
	int res = csp_can_socketcan_open_and_add_interface(device, CSP_IF_CAN_DEFAULT_NAME, bitrate, promisc, &return_iface);
	return (res == CSP_ERR_NONE) ? return_iface : NULL;
}

int csp_can_socketcan_stop(csp_iface_t *iface)
{
	can_context_t * ctx = iface->driver_data;

	int error = pthread_cancel(ctx->rx_thread);
	if (error != 0) {
		csp_log_error("%s[%s]: pthread_cancel() failed, error: %s", __FUNCTION__, ctx->name, strerror(errno));
		return CSP_ERR_DRIVER;
	}
	error = pthread_join(ctx->rx_thread, NULL);
	if (error != 0) {
		csp_log_error("%s[%s]: pthread_join() failed, error: %s", __FUNCTION__, ctx->name, strerror(errno));
		return CSP_ERR_DRIVER;
	}
        socketcan_free(ctx);
	return CSP_ERR_NONE;
}
