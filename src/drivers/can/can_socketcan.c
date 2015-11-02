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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

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
#include <bits/socket.h>

#include <csp/csp.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/arch/csp_thread.h>
#include <csp/interfaces/csp_if_can.h>

#include "can.h"

/* Number of mailboxes */
#define MBOX_NUM 5

/* These constants are not defined for Blackfin */
#if !defined(PF_CAN) && !defined(AF_CAN)
	#define PF_CAN 29
	#define AF_CAN PF_CAN
#endif

#define MAX_SUPPORTED_CAN_INSTANCES 3



/** Callback functions */
can_tx_callback_t txcb;
can_rx_callback_t rxcb;

/* Mailbox pool */
typedef enum {
	MBOX_FREE = 0,
	MBOX_USED = 1,
} mbox_state_t;

typedef struct csp_can_socket_t {
	int can_socket;         /** SocketCAN socket handle */
	csp_iface_t *csp_if_can;
} csp_can_socket_t;

typedef struct {
	csp_thread_handle_t thread;  		/** Thread handle */
	csp_bin_sem_handle_t signal_sem;   	/** Signalling semaphore */
	mbox_state_t state;		/** Thread state */
	struct can_frame frame;	/** CAN Frame */
	csp_can_socket_t * csp_can_socket;
	csp_bin_sem_handle_t *mbox_pool_sem;         /** Mailbox pool semaphore */
} mbox_t;

typedef struct can_iface_ctx_t {
	csp_can_socket_t csp_can_socket;
	mbox_t mbox[MBOX_NUM];			/** List of mailboxes */
	csp_bin_sem_handle_t mbox_pool_sem;			/** Mailbox pool semaphore */
} can_iface_ctx_t;

can_iface_ctx_t can_iface_ctx[MAX_SUPPORTED_CAN_INSTANCES];

/* Mailbox thread */
CSP_DEFINE_TASK(mbox_tx_thread) {

	/* Set thread parameters */
	mbox_t * m = (mbox_t *)param;
	csp_can_socket_t *csp_can_socket = m->csp_can_socket;
	uint32_t id;

	while (1) {

		/* Wait for a new packet to process */
		csp_bin_sem_wait(&(m->signal_sem), CSP_MAX_DELAY);

		/* Send frame */
		int tries = 0, error = CAN_NO_ERROR;
		while (write(csp_can_socket->can_socket, &m->frame, sizeof(m->frame)) != sizeof(m->frame)) {
			if (++tries < 1000 && errno == ENOBUFS) {
				/* Wait 10 ms and try again */
				csp_sleep_ms(10);
			} else {
				csp_log_error("write: %s", strerror(errno));
				error = CAN_ERROR;
				break;
			}
		}

		id = m->frame.can_id;

		/* Free mailbox */
		csp_bin_sem_wait(m->mbox_pool_sem, CSP_MAX_DELAY);
		m->state = MBOX_FREE;
		csp_bin_sem_post(m->mbox_pool_sem);

		/* Call tx callback */
		if (txcb) txcb(csp_can_socket->csp_if_can, id, error, NULL);

	}

	/* We should never reach this point */
	abort();

}

CSP_DEFINE_TASK(mbox_rx_thread) {

	csp_can_frame_t csp_can_frame;
	struct can_frame *frame;
	int nbytes;
	csp_can_socket_t *csp_can_socket = (csp_can_socket_t *) param;
	csp_can_frame.interface = csp_can_socket->csp_if_can;
	frame = (struct can_frame*) &csp_can_frame;

	while (1) {
		/* Read CAN frame */
		nbytes = read(csp_can_socket->can_socket, frame, sizeof(*frame));

		if (nbytes < 0) {
			csp_log_error("read: %s", strerror(errno));
			break;
		}

		if (nbytes != sizeof(*frame)) {
			csp_log_warn("Read incomplete CAN frame");
			continue;
		}

		/* Frame type */
		if (frame->can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG) || !(frame->can_id & CAN_EFF_FLAG)) {
			/* Drop error and remote frames */
			csp_log_warn("Discarding ERR/RTR/SFF frame");
		} else {
			/* Strip flags */
			frame->can_id &= CAN_EFF_MASK;
		}

		/* Call RX callback */
		if (rxcb) rxcb(&csp_can_frame, NULL);
	}

	/* We should never reach this point */
	abort();

}

int can_mbox_init(can_iface_ctx_t *iface_ctx) {

	int i;
	mbox_t *m, *mbox;

	mbox = iface_ctx->mbox;

	for (i = 0; i < MBOX_NUM; i++) {
		m = &mbox[i];
		m->state = MBOX_FREE;
		m->csp_can_socket = &iface_ctx->csp_can_socket;
		m->mbox_pool_sem = &iface_ctx->mbox_pool_sem;

		/* Init signal semaphore */
		if (csp_bin_sem_create(&(m->signal_sem)) != CSP_SEMAPHORE_OK) {
			csp_log_error("sem create");
			return -1;
		} else {
			/* Take signal semaphore so the thread waits for tx data */
			csp_bin_sem_wait(&(m->signal_sem), CSP_MAX_DELAY);
		}

		/* Create mailbox */
		if(csp_thread_create(mbox_tx_thread, (signed char *)"mbox_tx", 1024, (void *)m, 3,  &m->thread) != CSP_ERR_NONE) { //TODO: Adjust priority
			csp_log_error("thread creation");
			return -1;
		}
	}

	/* Init mailbox pool semaphore */
	csp_bin_sem_create(&iface_ctx->mbox_pool_sem);
	csp_bin_sem_post(&iface_ctx->mbox_pool_sem);
	return 0;

}

int can_send(csp_iface_t *csp_if_can, can_id_t id, uint8_t data[], uint8_t dlc, CSP_BASE_TYPE * task_woken) {

	int i, found = 0;
	mbox_t * m;
	can_iface_ctx_t *iface_ctx;
	csp_bin_sem_handle_t *mbox_pool_sem;

	if (dlc > 8)
		return -1;

	iface_ctx = (can_iface_ctx_t *)csp_if_can->driver;
	if (NULL == iface_ctx) {
		return -1;
	}
	mbox_pool_sem = &iface_ctx->mbox_pool_sem;

	/* Find free mailbox */
	csp_bin_sem_wait(mbox_pool_sem, CSP_MAX_DELAY);
	for (i = 0; i < MBOX_NUM; i++) {
		m = &(iface_ctx->mbox[i]);
		if(m->state == MBOX_FREE) {
			m->state = MBOX_USED;
			found = 1;
			break;
		}
	}
	csp_bin_sem_post(mbox_pool_sem);

	if (!found)
		return -1;

	/* Copy identifier */
	m->frame.can_id = id | CAN_EFF_FLAG;

	/* Copy data to frame */
	for (i = 0; i < dlc; i++)
		m->frame.data[i] = data[i];

	/* Set DLC */
	m->frame.can_dlc = dlc;

	/* Signal thread to start */
	csp_bin_sem_post(&(m->signal_sem));

	return 0;

}
static unsigned char total_interfaces = 0;

can_iface_ctx_t * get_available_interface_ctx(void)
{
	can_iface_ctx_t *iface_ctx;
	if(total_interfaces < MAX_SUPPORTED_CAN_INSTANCES) {
		iface_ctx = &can_iface_ctx[total_interfaces];
		total_interfaces++;
		return iface_ctx;
	}
	return NULL;
}

int can_init(csp_iface_t *csp_if_can, uint32_t id, uint32_t mask, can_tx_callback_t atxcb, can_rx_callback_t arxcb, struct csp_can_config *conf) {

	struct ifreq ifr;
	struct sockaddr_can addr;
	csp_thread_handle_t rx_thread;
	int *can_socket;
	can_iface_ctx_t *iface_ctx;

	iface_ctx = get_available_interface_ctx();
	if (iface_ctx == NULL) {
		return -1;
	}

	csp_if_can->driver = (void *)iface_ctx;
	iface_ctx->csp_can_socket.csp_if_can = csp_if_can;
	can_socket = &iface_ctx->csp_can_socket.can_socket;

	csp_assert(conf && conf->ifc);

	txcb = atxcb;
	rxcb = arxcb;

	/* Create socket */
	if ((*can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		csp_log_error("socket: %s", strerror(errno));
		return -1;
	}

	/* Locate interface */
	strncpy(ifr.ifr_name, conf->ifc, IFNAMSIZ - 1);
	if (ioctl(*can_socket, SIOCGIFINDEX, &ifr) < 0) {
		csp_log_error("ioctl: %s", strerror(errno));
		return -1;
	}

	/* Bind the socket to CAN interface */
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(*can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		csp_log_error("bind: %s", strerror(errno));
		return -1;
	}

	/* Set promiscuous mode */
	if (mask) {
		struct can_filter filter;
		filter.can_id   = id;
		filter.can_mask = mask;
		if (setsockopt(*can_socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
			csp_log_error("setsockopt: %s", strerror(errno));
			return -1;
		}
	}

	/* Create receive thread */
	if(csp_thread_create(mbox_rx_thread, (signed char *)"mbox_rx", 1024, (void *)&iface_ctx->csp_can_socket, 3,  &rx_thread) != CSP_ERR_NONE) { //TODO: Adjust priority
		csp_log_error("thread creation");
		return -1;
	}

	/* Create mailbox pool */
	if (can_mbox_init(iface_ctx) != 0) {
		csp_log_error("Failed to create tx thread pool");
		return -1;
	}

	return 0;

}
