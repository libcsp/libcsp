#include "can_grlibCan.h"

#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

int stack_ptr = 0;
uint32_t csp_can_sent = 0;
uint32_t csp_can_recv = 0;
char stacks[LIBCSP_STACK_POOL][LIBCSP_STACK_SZ];

csp_iface_t * csp_can_grlibCan_init(can_context_t * ctx, grlibCan_dev_t * device, unsigned int node_id, int baudrate, bool promics) {
	csp_iface_t * return_iface;
	ctx->kill = false;
	int res = csp_can_grlibCan_open_and_add_interface(ctx, device, CSP_IF_CAN_DEFAULT_NAME, node_id, baudrate, promics, &return_iface);
	return (res == CSP_ERR_NONE) ? return_iface : NULL;
}

int csp_can_grlibCan_open_and_add_interface(can_context_t * ctx, grlibCan_dev_t * device, const char * ifname, unsigned int node_id, int baudrate, bool promisc, csp_iface_t ** return_iface) {
	if (ctx == NULL || device == NULL) {
		return CSP_ERR_INVAL;
	}

	if (ifname == NULL) {
		ifname = CSP_IF_CAN_DEFAULT_NAME;
	}

	/* Turn off loopback mode */
	grlibCan_config_t config;
	grlibCan_copyConfig(device, &config);

	config.conf &= ~((1 << 7) | (1 << 6));
	config.rxCtrlReg |= (1 << 3);

	if (baudrate > 0) {
		config.nomBdRate = baudrate;
		config.dataBdRate = baudrate;
	}

	grlibCan_applyConfig(device, &config);
	
	strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
	ctx->device = device;
	ctx->iface.name = ctx->name;
	ctx->iface.addr = node_id;
	ctx->iface.netmask = 32;
	ctx->iface.interface_data = &ctx->ifdata;
	ctx->iface.driver_data = ctx;
	ctx->ifdata.tx_func = csp_can_grlibCan_tx_frame;
	ctx->ifdata.pbufs = NULL;

	int res = csp_can_add_interface(&ctx->iface);
	if (res != CSP_ERR_NONE) {
		return res;
	}

	if (stack_ptr >= LIBCSP_STACK_POOL) {
		csp_can_remove_interface(&ctx->iface);
		return CSP_ERR_NOMEM;
	}

	if (beginthread(grlibCan_rx_thread, 1, stacks[stack_ptr++], LIBCSP_STACK_SZ, (void *)ctx) < 0) {
		csp_can_remove_interface(&ctx->iface);
		return CSP_ERR_NOMEM;
	}

	if (return_iface) {
		*return_iface = &ctx->iface;
	}

	return CSP_ERR_NONE;
}

void grlibCan_rx_thread(void * arg) {
	printf("RX thread started\n");

	can_context_t * ctx = (can_context_t *)arg;
	uint32_t pending = 0;

	/* Size of largest Can frame is at most 5 grlibCan_msg_t */
	grlibCan_msg_t msgs[5];

	while (1) {
		pending = 0;
		if (ctx->kill) {
			break;
		}

		int ret = grlibCan_recvSync(ctx->device, msgs, 1, &pending);
		if (pending != 0) {
			ret = grlibCan_recvSync(ctx->device, msgs, pending, &pending);
		}
		csp_can_recv += ret;

		if (ret == 0) {
			continue;
		}

		/* CSP accepts only standard length frames */
		if ((msgs->frame.stat >> 28) > 8) {
			/* Redirect frame to standard reception queue */
			printf("Received frame to long\n");
			continue;
		}

		/* CSP accepts only extended ID */
		if ((msgs->frame.head & (1 << 31)) == 0) {
			/* Redirect frame to standard reception queue */
			continue;
		}

		/* CSP does not support RTR*/
		if ((msgs->frame.head & (1 << 30)) != 0) {
			/* Redirect frame to standard reception queue */
			continue;
		}

		csp_can_rx(&ctx->iface, msgs->frame.head & ~(1 << 31), msgs->frame.payload, msgs->frame.stat >> 28, NULL);
	}

	csp_can_remove_interface(&ctx->iface);
	endthread();
}

int csp_can_grlibCan_tx_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc) {
	if (dlc > 8) {
		return CSP_ERR_INVAL;
	}

	can_context_t * ctx = (can_context_t *)driver_data;

	grlibCan_msg_t packet;
	packet.frame.head = id | (1 << 31);
	packet.frame.stat = dlc << 28;

	memcpy((void *)packet.frame.payload, (void *)data, dlc);

	if (grlibCan_transmitAsync(ctx->device, &packet, 1) != 1) {
		return CSP_ERR_TX;
	}
	return CSP_ERR_NONE;
}

/* Currently always receive anything */
int csp_can_grlibCan_set_promisc(const bool promisc, can_context_t * ctx) {
	grlibCan_config_t config;
	grlibCan_copyConfig(ctx->device, &config);
	/* Do not compare on any bits - accept anything */
	config.rxAccMask = 0;
	grlibCan_applyConfig(ctx->device, &config);
	return CSP_ERR_NONE;
}

int csp_can_grlibCan_stop(csp_iface_t * iface) {
	((can_context_t *)iface->driver_data)->kill = true;
	return CSP_ERR_NONE;
}
