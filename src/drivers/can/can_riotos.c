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

#include <stdlib.h>
#include <csp/csp.h>
#include <csp/arch/csp_thread.h>

#include "can/can.h"
#include "can/can_trx.h"
#include "can/conn/raw.h"
#include "tja1042.h"
#include "thread.h"

#include "csp/csp_debug.h"
#include "errno.h"

#include "string.h"

static char thread_stack[THREAD_STACKSIZE_MAIN];

//This is a standard CAN transceiver and will work with most models
tja1042_trx_t tja1042 = { .trx.driver = &tja1042_driver,
                          .stb_pin = GPIO_PIN(0,0)
};

// CAN interface data, state, etc.
typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;
	csp_can_interface_data_t ifdata;
	kernel_pid_t rx_thread;
	int ifnum;
} can_context_t;

static int csp_can_tx_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc)
{
	if (dlc > 8) {
		return CSP_ERR_INVAL;
	}

	struct can_frame frame = {
		.can_id = id | CAN_EFF_FLAG,
        .can_dlc = dlc
	};
    memcpy(frame.data, data, dlc);

	can_context_t *ctx = driver_data;
	conn_can_raw_t conn;
    conn_can_raw_create(&conn, NULL, 0, ctx->ifnum, 0);
    int ret = conn_can_raw_send(&conn, &frame, 0);
    if (ret < 0) {
        csp_log_warn("%s[%s]: write() failed, errno %d", __FUNCTION__, ctx->name, ret);
		return CSP_ERR_TX;
    }
	conn_can_raw_close(&conn);

	return CSP_ERR_NONE;
}

static void *socketcan_rx_thread(void * arg)
{
	can_context_t *ctx = (can_context_t *)arg;
	conn_can_raw_t conn;
	struct can_frame frame;

	//setup mask to receive all messages
	struct can_filter filters[1];
	filters[0].can_id = 0;
	filters[0].can_mask = 0;
    conn_can_raw_create(&conn, filters, 1, ctx->ifnum, 0);

	while (1) {
		int nbytes = conn_can_raw_recv(&conn, &frame, 0);
		if (nbytes < 0) {
			csp_log_error("%s[%s]: read() failed, errno %d: %s", __FUNCTION__, ctx->name, errno, strerror(errno));
			continue;
		}
		if (nbytes != sizeof(struct can_frame)) {
			csp_log_warn("%s[%s]: Read incomplete CAN frame, size: %d, expected: %u bytes", __FUNCTION__, ctx->name, nbytes, (unsigned int) sizeof(frame));
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

		//Call RX callbacsp_can_rx_frameck 
		csp_can_rx(&ctx->iface, frame.can_id, frame.data, frame.can_dlc, NULL);
	}

	/* We should never reach this point */
	return NULL;
}


int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, int bitrate, bool promisc, csp_iface_t ** return_iface) {
	//this function does not implement promisc
	if (ifname == NULL) {
		ifname = CSP_IF_CAN_DEFAULT_NAME;
	}

	csp_log_info("INIT %s: device: [%s], bitrate: %d, promisc: %d",
			ifname, device, bitrate, promisc);

	int res = can_trx_init((can_trx_t *)&tja1042);
    if (res < 0) {
        csp_log_error("%s[%s]: can failed, error: %d", __FUNCTION__, ifname, res);
        return CSP_ERR_DRIVER;
    }

	can_context_t * ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		return CSP_ERR_NOMEM;
	}

	if(strcmp(device, "can0") == 0) {
		ctx->ifnum = 0;
	} else if(strcmp(device, "can1") == 0) {
		ctx->ifnum = 1;
	} else if(strcmp(device, "can2") == 0) {
		ctx->ifnum = 2;
	} else {
		csp_log_error("%s[%s]: unknow device: %s   must be can0, can1, or can2", __FUNCTION__, ifname, device);
        return CSP_ERR_DRIVER;
	}

	strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
	ctx->iface.name = ctx->name;
	ctx->iface.interface_data = &ctx->ifdata;
	ctx->iface.driver_data = ctx;
	ctx->ifdata.tx_func = csp_can_tx_frame;

	/* Add interface to CSP */
    res = csp_can_add_interface(&ctx->iface);
	if (res != CSP_ERR_NONE) {
		csp_log_error("%s[%s]: csp_can_add_interface() failed, error: %d", __FUNCTION__, ctx->name, res);
		return res;
	}

	/* Create receive thread */
	ctx->rx_thread = thread_create(thread_stack, THREAD_STACKSIZE_MAIN,
                                       THREAD_PRIORITY_MAIN - 1,
                                       THREAD_CREATE_STACKTEST, socketcan_rx_thread,
                                       (void*)ctx, "can_receive_thread");
	if (ctx->rx_thread < 0) {
		csp_log_error("%s[%s]: pthread_create() failed, error: %d", __FUNCTION__, ctx->name, ctx->rx_thread);
		return CSP_ERR_NOMEM;
	}

	if (return_iface) {
		*return_iface = &ctx->iface;
	}

	return CSP_ERR_NONE;
}