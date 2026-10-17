#include <csp/interfaces/csp_if_can.h>

#include <csp/csp.h>
#include <csp/drivers/can_zephyr.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(csp_can_driver);

#define CSP_CAN_RX_MSGQ_TIME_OUT K_MSEC(1000)
#define CSP_CAN_TX_TIME_OUT      K_MSEC(100)
#define CSP_CAN_RX_THREAD_JOIN_TIME_OUT K_MSEC(1500)
#define CSP_CAN_STOP_EVENT (0x01)

/* A CSP CAN interface sends and receives one frame format */
typedef enum {
	CSP_CAN_MODE_CLASSIC,
	CSP_CAN_MODE_FD,
} csp_can_mode_t;

/**
 * CAN interface data
*/
typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;
	csp_can_interface_data_t ifdata;
	csp_can_mode_t mode;
	const struct device * device;
	struct k_msgq rx_msgq;
	uint8_t rx_msgq_buf[CONFIG_CSP_CAN_RX_MSGQ_DEPTH * sizeof(struct can_frame)];
	struct k_thread rx_thread;
	int filter_id;
	struct k_event stop_can_event;
} can_context_t;

static K_THREAD_STACK_ARRAY_DEFINE(rx_stack,
			CONFIG_CSP_CAN_RX_THREAD_NUM, CONFIG_CSP_CAN_RX_THREAD_STACK_SIZE);
static uint8_t rx_thread_idx = 0;

/* Frame format of a received frame */
static csp_can_mode_t csp_can_frame_mode(const struct can_frame * frame) {

	if (frame->flags & CAN_FRAME_FDF) {
		return CSP_CAN_MODE_FD;
	}
	return CSP_CAN_MODE_CLASSIC;
}

static void csp_can_rx_thread(void * arg1, void * arg2, void * arg3) {

	int ret;
	struct can_frame frame;
	struct k_msgq * rx_msgq = arg1;
	csp_iface_t * iface = arg2;
	struct k_event * stop_can_event = arg3;
	can_context_t * ctx = iface->driver_data;

	while (true) {
		/*
		 * Check if there is a need to stop the RX thread based
		 * on a user's request.
		*/
		if (k_event_wait(stop_can_event, CSP_CAN_STOP_EVENT, false, K_NO_WAIT) != 0) {
			break;
		}

		/* Read CAN frame */
		ret = k_msgq_get(rx_msgq, &frame, CSP_CAN_RX_MSGQ_TIME_OUT);
		if (ret == -EAGAIN) {
			continue;
		} else if (ret < 0) {
			LOG_ERR("[%s] k_msgq_get() failed, errno %d", iface->name, ret);
			break;
		}

		/* Drop frames with invalid size field */
		if (!(frame.flags & CAN_FRAME_FDF) && (frame.dlc > CAN_MAX_DLC)) {
			LOG_WRN("[%s] discarding invalid size frame", iface->name);
			continue;
		}

		/* An interface only takes frames of its own format */
		if (csp_can_frame_mode(&frame) != ctx->mode) {
			iface->frame++;
			continue;
		}

		/* CSP requires extended frame format, drop it. */
		if (!(frame.flags & CAN_FRAME_IDE)) {
			LOG_WRN("[%s] discarding Standard ID frame", iface->name);
			continue;
		}

		/* CSP uses data frame only, drop it. */
		if (frame.flags & CAN_FRAME_RTR) {
			LOG_WRN("[%s] discarding RTR frame", iface->name);
			continue;
		}

		/* The DLC of a classic frame is its byte count, a CAN FD DLC is a code */
		uint8_t data_size = (frame.flags & CAN_FRAME_FDF) ? can_dlc_to_bytes(frame.dlc) : frame.dlc;

		/* Call the common CSP CAN RX function. */
		switch (ctx->mode) {
			case CSP_CAN_MODE_CLASSIC:
				csp_can_rx(iface, frame.id, frame.data, data_size, 0, NULL);
				break;
			case CSP_CAN_MODE_FD:
				csp_canfd_rx(iface, frame.id, frame.data, data_size, 0, NULL);
				break;
		}
	}
}

static int csp_can_tx_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc, const csp_packet_t *packet) {

	int ret = CSP_ERR_NONE;
	struct can_frame frame = {0};
	can_context_t * ctx = driver_data;
	(void)packet;

	if (dlc > CAN_MAX_DLEN) {
		ret = CSP_ERR_INVAL;
		goto end;
	}

	frame.id = id;
	frame.dlc = dlc;
	frame.flags = CAN_FRAME_IDE;
	memcpy(frame.data, data, dlc);

	ret = can_send(ctx->device, &frame, CSP_CAN_TX_TIME_OUT, NULL, NULL);
	if (ret < 0) {
		LOG_ERR("[%s] can_send() failed, errno %d", ctx->name, ret);
	}

end:
	return ret;
}

#if defined(CONFIG_CAN_FD_MODE)
static int csp_canfd_tx_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t data_size, const csp_packet_t *packet) {

	int ret = CSP_ERR_NONE;
	struct can_frame frame = {0};
	can_context_t * ctx = driver_data;
	(void)packet;

	if (data_size > CSP_CANFD_FRAME_SIZE) {
		ret = CSP_ERR_INVAL;
		goto end;
	}

	/* Bit Rate Switch: send the data phase at the CAN FD data bitrate */
	frame.id = id;
	frame.dlc = can_bytes_to_dlc(data_size);
	frame.flags = CAN_FRAME_IDE | CAN_FRAME_FDF | CAN_FRAME_BRS;
	memcpy(frame.data, data, data_size);

	ret = can_send(ctx->device, &frame, CSP_CAN_TX_TIME_OUT, NULL, NULL);
	if (ret < 0) {
		LOG_ERR("[%s] can_send() failed, errno %d", ctx->name, ret);
	}

end:
	return ret;
}
#endif

static int csp_can_add_iface(can_context_t * ctx) {

	switch (ctx->mode) {
		case CSP_CAN_MODE_CLASSIC:
			return csp_can_add_interface(&ctx->iface);
		case CSP_CAN_MODE_FD:
			return csp_canfd_add_interface(&ctx->iface);
	}

	return CSP_ERR_INVAL;
}

static int csp_can_remove_iface(can_context_t * ctx) {

	switch (ctx->mode) {
		case CSP_CAN_MODE_CLASSIC:
			return csp_can_remove_interface(&ctx->iface);
		case CSP_CAN_MODE_FD:
			return csp_canfd_remove_interface(&ctx->iface);
	}

	return CSP_ERR_INVAL;
}

static void csp_can_remove_rx_filter(can_context_t * ctx) {

	can_remove_rx_filter(ctx->device, ctx->filter_id);
	ctx->filter_id = -1;
}

static int csp_can_finish_rx_thread(can_context_t * ctx) {

	int ret;

	k_event_set(&ctx->stop_can_event, CSP_CAN_STOP_EVENT);

	ret = k_thread_join(&ctx->rx_thread, CSP_CAN_RX_THREAD_JOIN_TIME_OUT);
	if (ret < 0) {
		LOG_WRN("[%s] Failed to finish the RX thread, but will continue the cleanup. error: %d",
			 ctx->name, ret);
	}

	k_event_clear(&ctx->stop_can_event, CSP_CAN_STOP_EVENT);

	rx_thread_idx--;

	k_msgq_purge(&ctx->rx_msgq);
	k_msgq_cleanup(&ctx->rx_msgq);

	return ret;
}

static int csp_can_open(const struct device * device, const char * ifname,
						uint16_t address, uint32_t bitrate, csp_can_mode_t mode,
						uint16_t filter_addr, uint16_t filter_mask,
						csp_iface_t ** return_iface) {

	int ret;
	k_tid_t rx_tid;
	can_context_t * ctx = NULL;
	const char * name;

	if (device == NULL) {
		ret = CSP_ERR_INVAL;
		goto end;
	}

	name = ifname ? ifname : device->name;

	if (rx_thread_idx >= CONFIG_CSP_CAN_RX_THREAD_NUM) {
		LOG_ERR("[%s] No more RX thread can be created. (MAX: %d) Please check CONFIG_CSP_CAN_RX_THREAD_NUM.",
			 name, CONFIG_CSP_CAN_RX_THREAD_NUM);
		ret = CSP_ERR_DRIVER;
		goto end;
	}

	/*
	 * TODO:
	 * In the current implementation, we use k_alloc() to allocate the memory
	 * for CAN context like socketcan implementation.
	 * However, we plan to remove dynamic memory allocations as described in
	 * the issue below.
	 * - https://github.com/libcsp/libcsp/issues/460
	 * And in Zephyr, the default heap memory size is 0, so we need to set a
	 * value using CONFIG_HEAP_MEM_POOL_SIZE.
	 */
	ctx = k_calloc(1, sizeof(can_context_t));
	if (ctx == NULL) {
		LOG_ERR("[%s] Failed to allocate %zu bytes from the system heap", 
			  name, sizeof(can_context_t));
		ret = CSP_ERR_NOMEM;
		goto end;
	}

	/* Set the each parameter to CAN context. */
	strncpy(ctx->name, name, sizeof(ctx->name) - 1);
	ctx->iface.name = ctx->name;
	ctx->iface.addr = address;
	ctx->iface.interface_data = &ctx->ifdata;
	ctx->iface.driver_data = ctx;
	ctx->ifdata.tx_func = csp_can_tx_frame;
#if defined(CONFIG_CAN_FD_MODE)
	if (mode == CSP_CAN_MODE_FD) {
		ctx->ifdata.tx_func = csp_canfd_tx_frame;
	}
#endif
	ctx->ifdata.pbufs = NULL;
	ctx->mode = mode;
	ctx->device = device;
	ctx->filter_id = -1;
	k_event_init(&ctx->stop_can_event);

	LOG_DBG("INIT %s: device: [%s], local address: %d, bitrate: %d: filter add: %d, filter mask: 0x%04x",
			  ctx->name, device->name, address, bitrate, filter_addr, filter_mask);

	/* Initialize the RX message queue */
	k_msgq_init(&ctx->rx_msgq, ctx->rx_msgq_buf,
						sizeof(struct can_frame), CONFIG_CSP_CAN_RX_MSGQ_DEPTH);

	/* Set Bit rate */
	ret = can_set_bitrate(device, bitrate);
	if (ret < 0) {
		LOG_ERR("[%s] can_set_bitrate() failed, error: %d", ctx->name, ret);
		goto cleanup_heap;
	}

#if defined(CONFIG_CAN_FD_MODE)
	if (mode == CSP_CAN_MODE_FD) {
		ret = can_set_bitrate_data(device, CSP_CANFD_DATA_BITRATE);
		if (ret < 0) {
			LOG_ERR("[%s] can_set_bitrate_data() failed, error: %d", ctx->name, ret);
			goto cleanup_heap;
		}

		ret = can_set_mode(device, CAN_MODE_FD);
		if (ret < 0) {
			LOG_ERR("[%s] can_set_mode(CAN_MODE_FD) failed, error: %d", ctx->name, ret);
			goto cleanup_heap;
		}
	}
#endif

	/* Set RX filter */
	ret = csp_can_set_rx_filter(&ctx->iface, filter_addr, filter_mask);
	if (ret < 0) {
		LOG_ERR("[%s] csp_can_add_rx_filter() failed, error: %d", ctx->name, ret);
		goto cleanup_heap;
	}

	/* Add interface to CSP */
	ret = csp_can_add_iface(ctx);
	if (ret != CSP_ERR_NONE) {
		LOG_ERR("[%s] adding interface failed, error: %d", ctx->name, ret);
		goto cleanup_filter;
	}

	/* Create receive thread */
	rx_tid = k_thread_create(&ctx->rx_thread, rx_stack[rx_thread_idx],
					 K_THREAD_STACK_SIZEOF(rx_stack[rx_thread_idx]),
					 (k_thread_entry_t)csp_can_rx_thread,
					 &ctx->rx_msgq, &ctx->iface, &ctx->stop_can_event,
					 CONFIG_CSP_CAN_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!rx_tid) {
		LOG_ERR("[%s] k_thread_create() failed", ctx->name);
		ret = CSP_ERR_DRIVER;
		goto cleanup_iface;
	}
	rx_thread_idx++;

	/* Enable CAN */
	ret = can_start(device);
	if (ret < 0) {
		LOG_ERR("[%s] can_start() failed, error: %d", ctx->name, ret);
		goto cleanup_thread;
	}

	if (return_iface) {
		*return_iface = &ctx->iface;
	}

	return ret;

	/*
	 * The following section is for restoring acquired resources when
	 * something fails. Unfortunately, we can't take any action if the
	 * restoration process fails, so we proceed with the remaining
	 * cleanup. In addition to this, we've chosen not to restore the
	 * CAN bit rate. If this causes any issues, please open an issue
	 * on GitHub.
	 */
cleanup_thread:
	(void)csp_can_finish_rx_thread(ctx);

cleanup_iface:
	(void)csp_can_remove_iface(ctx);

cleanup_filter:
	csp_can_remove_rx_filter(ctx);

cleanup_heap:
	k_free(ctx);

end:
	return ret;
}

int csp_can_open_and_add_interface(const struct device * device, const char * ifname,
				    uint16_t address, uint32_t bitrate,
				    uint16_t filter_addr, uint16_t filter_mask,
				    csp_iface_t ** return_iface) {

	return csp_can_open(device, ifname, address, bitrate, CSP_CAN_MODE_CLASSIC, filter_addr, filter_mask, return_iface);
}

int csp_canfd_open_and_add_interface(const struct device * device, const char * ifname,
				      uint16_t address,
				      uint16_t filter_addr, uint16_t filter_mask,
				      csp_iface_t ** return_iface) {

#if defined(CONFIG_CAN_FD_MODE)
	return csp_can_open(device, ifname, address, CSP_CANFD_BITRATE, CSP_CAN_MODE_FD, filter_addr, filter_mask, return_iface);
#else
	LOG_ERR("CAN FD requires CONFIG_CAN_FD_MODE");
	return CSP_ERR_NOTSUP;
#endif
}

int csp_can_set_rx_filter(csp_iface_t * iface, uint16_t filter_addr, uint16_t filter_mask) {

	int ret;
	struct can_filter filter = {
		.flags = CAN_FILTER_IDE,
	};
	can_context_t * ctx;

	if ((iface == NULL) || (iface->driver_data == NULL)) {
		ret = CSP_ERR_INVAL;
		goto end;
	}

	ctx = iface->driver_data;

	/* If a filter is already set, delete it. */
	if (ctx->filter_id >= 0) {
		can_remove_rx_filter(ctx->device, ctx->filter_id);
	}

	if (csp_conf.version == 1) {
		filter.id = CFP_MAKE_DST(filter_addr);
		filter.mask = CFP_MAKE_DST(filter_mask);
	} else {
		filter.id = filter_addr << CFP2_DST_OFFSET;
		filter.mask = filter_mask << CFP2_DST_OFFSET;
	}

	ret = can_add_rx_filter_msgq(ctx->device, &ctx->rx_msgq, &filter);
	if (ret < 0) {
		LOG_ERR("[%s] can_add_rx_filter_msgq() failed, error: %d", iface->name, ctx->filter_id);
		goto end;
	}
	ctx->filter_id = ret;

end:
	return ret;
}

int csp_can_stop(csp_iface_t * iface) {

	int ret;
	can_context_t * ctx;

	if ((iface == NULL) || (iface->driver_data == NULL)) {
		ret = CSP_ERR_INVAL;
		goto end;
	}

	ctx = iface->driver_data;

	ret = can_stop(ctx->device);
	if (ret < 0) {
		LOG_WRN("[%s] can_stop() failed, but will continue the cleannp. error: %d", iface->name, ret);
	}

	(void)csp_can_finish_rx_thread(ctx);

	(void)csp_can_remove_iface(ctx);

	csp_can_remove_rx_filter(ctx);

	LOG_DBG("Stop CAN interface: %s. ret: %d", iface->name, ret);

	k_free(ctx);

end:
	return ret;
}
