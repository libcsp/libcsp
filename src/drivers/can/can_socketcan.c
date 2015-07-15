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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
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
#include <bits/socket.h>

#include <csp/csp.h>
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

typedef struct {
	pthread_t thread;  		/** Thread handle */
	sem_t signal_sem;   	/** Signalling semaphore */
	mbox_state_t state;		/** Thread state */
	struct can_frame frame;	/** CAN Frame */
} mbox_t;

typedef struct {
    int can_socket;         /** SocketCAN socket handle */
    mbox_t mbox[MBOX_NUM];  /* List of mailboxes */
    sem_t mbox_sem;         /** Mailbox pool semaphore */
    csp_iface_t *csp_if_can;
    bool in_use;
} can_socket_info_t;

can_socket_info_t can_socket_info[MAX_SUPPORTED_CAN_INSTANCES];

can_socket_info_t * find_can_socket_info_by_mailbox(mbox_t *m)
{
    for (int i=0; i < MAX_SUPPORTED_CAN_INSTANCES; i++) {
        for (int j=0; j < MBOX_NUM; j++) {
            if (&can_socket_info[i].mbox[j] == m) {
                return &can_socket_info[i];
            }
        }
    }
    return NULL;
}

/* Mailbox thread */
static void * mbox_tx_thread(void * parameters) {

	/* Set thread parameters */
	mbox_t * m = (mbox_t *)parameters;
        can_socket_info_t *csi;
        int *can_socket;

	uint32_t id;

        csi = find_can_socket_info_by_mailbox(m);
        if (csi == NULL) {
            /* can this happen?? */
        }

        can_socket = &csi->can_socket;

	while (1) {

		/* Wait for a new packet to process */
		sem_wait(&(m->signal_sem));

		/* Send frame */
		int tries = 0, error = CAN_NO_ERROR;
		while (write(*can_socket, &m->frame, sizeof(m->frame)) != sizeof(m->frame)) {
			if (++tries < 1000 && errno == ENOBUFS) {
				/* Wait 10 ms and try again */
				usleep(10000);
			} else {
				csp_log_error("write: %s", strerror(errno));
				error = CAN_ERROR;
				break;
			}
		}

		id = m->frame.can_id;

		/* Free mailbox */
		sem_wait(&csi->mbox_sem);
		m->state = MBOX_FREE;
		sem_post(&csi->mbox_sem);
		
		/* Call tx callback */
		if (txcb) txcb(csi->csp_if_can, id, error, NULL);

	}

	/* We should never reach this point */
	pthread_exit(NULL);

}

static void * mbox_rx_thread(void * parameters) {

    struct can_frame *frame;
    int nbytes;
    can_socket_info_t *csi;
    csi = (can_socket_info_t *) parameters;
    rx_queue_element_t e = {
        .interface = csi->csp_if_can,
    };
    frame = (struct can_frame*) &e.frame;

	while (1) {
		/* Read CAN frame */
		nbytes = read(csi->can_socket, frame, sizeof(*frame));

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
		if (rxcb) rxcb(&e, NULL);
	}

	/* We should never reach this point */
	pthread_exit(NULL);

}

int can_mbox_init(can_socket_info_t *csi) {

	int i;
	mbox_t *m, *mbox;

        mbox = csi->mbox;

	for (i = 0; i < MBOX_NUM; i++) {
		m = &mbox[i];
		m->state = MBOX_FREE;

		/* Init signal semaphore */
		if (sem_init(&(m->signal_sem), 0, 1) != 0) {
			csp_log_error("sem_init: %s", strerror(errno));
			return -1;
		} else {
			/* Take signal semaphore so the thread waits for tx data */
			sem_wait(&(m->signal_sem));
		}

		/* Create mailbox */
		if (pthread_create(&m->thread, NULL, mbox_tx_thread, (void *)m) != 0) {
			csp_log_error("pthread_create: %s", strerror(errno));
			return -1;
		}
	}

	/* Init mailbox pool semaphore */
	sem_init(&csi->mbox_sem, 0, 1);
	return 0;

}

can_socket_info_t * find_can_socket_info(csp_iface_t *ifc)
{
    for (int i = 0; i < MAX_SUPPORTED_CAN_INSTANCES; i++)
    {
        /* first unused means no more sockets in use remains */
        if (!can_socket_info[i].in_use) {
            return NULL;
        }
        if (can_socket_info[i].csp_if_can == ifc) {
            return &can_socket_info[i];
        }
    }
    return NULL;
}

int can_send(csp_iface_t *csp_if_can, can_id_t id, uint8_t data[], uint8_t dlc, CSP_BASE_TYPE * task_woken) {

	int i, found = 0;
	mbox_t * m;
        can_socket_info_t *csi;
        sem_t *mbox_sem;

	if (dlc > 8)
		return -1;

        csi = find_can_socket_info(csp_if_can);
        if (NULL == csi) {
            return -1;
        }
        mbox_sem = &csi->mbox_sem;

	/* Find free mailbox */
	sem_wait(mbox_sem);
	for (i = 0; i < MBOX_NUM; i++) {
		m = &(csi->mbox[i]);
		if(m->state == MBOX_FREE) {
			m->state = MBOX_USED;
			found = 1;
			break;
		}
	}
	sem_post(mbox_sem);
	
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
	sem_post(&(m->signal_sem));

	return 0;

}
static unsigned char total_interfaces = 0;

can_socket_info_t * get_available_can_socket_info(void)
{
    can_socket_info_t *csi;
    if(total_interfaces < MAX_SUPPORTED_CAN_INSTANCES) {
        csi = &can_socket_info[total_interfaces];
        total_interfaces++;
        csi->in_use = true;
        return csi;
    }
    return NULL;
}

static void init_socket_info_in_use(void)
{
    static bool first_call = true;
    if (first_call) {
        for (int i=0; i < MAX_SUPPORTED_CAN_INSTANCES; i++){
            can_socket_info[i].in_use = false;
        }
        first_call = false;
    }
}

int can_init(csp_iface_t *csp_if_can, uint32_t id, uint32_t mask, can_tx_callback_t atxcb, can_rx_callback_t arxcb, struct csp_can_config *conf) {

	struct ifreq ifr;
	struct sockaddr_can addr;
	pthread_t rx_thread;
        int *can_socket;
        can_socket_info_t *socket_info;

        init_socket_info_in_use();

        socket_info = get_available_can_socket_info();
        if (socket_info == NULL) {
            return -1;
        }

        socket_info->csp_if_can = csp_if_can;
        can_socket = &socket_info->can_socket;

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
	if (pthread_create(&rx_thread, NULL, mbox_rx_thread, (void *)socket_info) != 0) {
		csp_log_error("pthread_create: %s", strerror(errno));
		return -1;
	}

	/* Create mailbox pool */
	if (can_mbox_init(socket_info) != 0) {
		csp_log_error("Failed to create tx thread pool");
		return -1;
	}

	return 0;

}
