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

/* SocketCAN driver */
#include <csp/drivers/can_socketcan.h>

#include <stdint.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/queue.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/socket.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

#ifdef CSP_HAVE_LIBSOCKETCAN
#include <libsocketcan.h>
#endif

static struct can_socketcan_s {
	int socket;
	csp_iface_t interface;
} socketcan[1] = {
	{
		.interface = {
			.name = "CAN",
			.nexthop = csp_can_tx,
			.mtu = CSP_CAN_MTU,
			.driver = &socketcan[0],
		},
	},
};

static void * socketcan_rx_thread(void * parameters)
{
	struct can_frame frame;
	int nbytes;

	while (1) {
		/* Read CAN frame */
		nbytes = read(socketcan[0].socket, &frame, sizeof(frame));
		if (nbytes < 0) {
			csp_log_error("read: %s", strerror(errno));
			continue;
		}

		if (nbytes != sizeof(frame)) {
			csp_log_warn("Read incomplete CAN frame");
			continue;
		}

		/* Frame type */
		if (frame.can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG) || !(frame.can_id & CAN_EFF_FLAG)) {
			/* Drop error and remote frames */
			csp_log_warn("Discarding ERR/RTR/SFF frame");
			continue;
		}

		/* Strip flags */
		frame.can_id &= CAN_EFF_MASK;

		/* Call RX callbacsp_can_rx_frameck */
		csp_can_rx(&socketcan[0].interface, frame.can_id, frame.data, frame.can_dlc, NULL);
	}

	/* We should never reach this point */
	pthread_exit(NULL);
}


int csp_can_tx_frame(csp_iface_t *interface, uint32_t id, const uint8_t * data, uint8_t dlc)
{
	struct can_frame frame;
	int i, tries = 0;

	if (dlc > 8)
		return -1;

	/* Copy identifier */
	frame.can_id = id | CAN_EFF_FLAG;

	/* Copy data to frame */
	for (i = 0; i < dlc; i++)
		frame.data[i] = data[i];

	/* Set DLC */
	frame.can_dlc = dlc;

	/* Send frame */
	while (write(socketcan[0].socket, &frame, sizeof(frame)) != sizeof(frame)) {
		if (++tries < 1000 && errno == ENOBUFS) {
			/* Wait 10 ms and try again */
			usleep(10000);
		} else {
			csp_log_error("write: %s", strerror(errno));
			break;
		}
	}

	return 0;
}

csp_iface_t * csp_can_socketcan_init(const char * ifc, int bitrate, int promisc)
{
	struct ifreq ifr;
	struct sockaddr_can addr;
	pthread_t rx_thread;

	printf("Init can interface %s\n", ifc);

#ifdef CSP_HAVE_LIBSOCKETCAN
	/* Set interface up */
	if (bitrate > 0) {
		can_do_stop(ifc);
		can_set_bitrate(ifc, bitrate);
		can_set_restart_ms(ifc, 100);
		can_do_start(ifc);
	}
#endif

	/* Create socket */
	if ((socketcan[0].socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		csp_log_error("socket: %s", strerror(errno));
		return NULL;
	}

	/* Locate interface */
	strncpy(ifr.ifr_name, ifc, IFNAMSIZ - 1);
	if (ioctl(socketcan[0].socket, SIOCGIFINDEX, &ifr) < 0) {
		csp_log_error("ioctl: %s", strerror(errno));
		return NULL;
	}

	/* Bind the socket to CAN interface */
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(socketcan[0].socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		csp_log_error("bind: %s", strerror(errno));
		return NULL;
	}

	/* Set filter mode */
	if (promisc == 0) {

		struct can_filter filter;
		filter.can_id = CFP_MAKE_DST(csp_get_address());
		filter.can_mask = CFP_MAKE_DST((1 << CFP_HOST_SIZE) - 1);

		if (setsockopt(socketcan[0].socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
			csp_log_error("setsockopt: %s", strerror(errno));
			return NULL;
		}

	}

	/* Create receive thread */
	if (pthread_create(&rx_thread, NULL, socketcan_rx_thread, NULL) != 0) {
		csp_log_error("pthread_create: %s", strerror(errno));
		return NULL;
	}

	csp_iflist_add(&socketcan[0].interface);

	return &socketcan[0].interface;
}
