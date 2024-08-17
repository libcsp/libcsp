#include "csp/drivers/usart.h"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(libcsp, CONFIG_LIBCSP_LOG_LEVEL);

typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_usart_callback_t rx_callback;
	void * user_data;
	csp_usart_fd_t fd;
	struct k_thread rx_thread;
	int cbuf_len;
	uint8_t cbuf[CONFIG_CSP_UART_RX_BUFFER_LENGTH + 1];
} usart_context_t;

static K_THREAD_STACK_ARRAY_DEFINE(uart_rx_stack,
								   CONFIG_CSP_UART_RX_THREAD_NUM, CONFIG_CSP_UART_RX_THREAD_STACK_SIZE);
static uint8_t uart_rx_thread_idx = 0;

K_MUTEX_DEFINE(uart_driver_lock);

void csp_usart_lock(void * driver_data) {
	k_mutex_lock(&uart_driver_lock, K_MSEC(CONFIG_CSP_MUTEX_TIMEOUT));
}

void csp_usart_unlock(void * driver_data) {
	k_mutex_unlock(&uart_driver_lock);
}

int csp_usart_write(csp_usart_fd_t fd, const void * data, size_t data_length) {
	unsigned char * buf = (unsigned char *)data;

	for (size_t i = 0; i < data_length; i++) {
		uart_poll_out(fd, buf[i]);
	}

	return data_length;
}

int csp_usart_zephyr_process_char(usart_context_t * ctx) {
	int ret;
	char recv_char;

	ret = uart_poll_in(ctx->fd, &recv_char);

	/* no more data */
	if (ret < 0) {
		/* if we have any data, hand it over */
		if (ctx->cbuf_len > 0) {
			ctx->rx_callback(ctx->user_data, ctx->cbuf, ctx->cbuf_len, NULL);
			ctx->cbuf_len = 0;
		}
		return ret;
	}

	/* got some data */

	/* FIXME: API shouldn't return > 0 but pl1011 does */
	if (ret > 0) LOG_WRN("[%s] uart_poll_in returned %d", ctx->name, ret);

	/* put data in the buf. We are sure we have room for it */
	ctx->cbuf[ctx->cbuf_len] = recv_char;
	ctx->cbuf_len++;

	/* if it's full, hand it over */
	if (ctx->cbuf_len >= CONFIG_CSP_UART_RX_BUFFER_LENGTH) {
		ctx->rx_callback(ctx->user_data, ctx->cbuf, ctx->cbuf_len, NULL);
		ctx->cbuf_len = 0;
	}

	return ret;
}

void csp_usart_rx_thread(void * arg1, void * arg2, void * arg3) {
	usart_context_t * ctx = arg1;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_DBG("[%s] start uart rx polling thread", ctx->name);

	while (true) {
		int ret = csp_usart_zephyr_process_char(ctx);

		if (ret < 0) {
			k_sleep(K_NSEC(CONFIG_CSP_UART_RX_INTERVAL));
		}
	}
}

int csp_usart_open(const csp_usart_conf_t * conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * return_fd) {
	if (rx_callback == NULL) {
		LOG_ERR("%s: No rx_callback function pointer provided\n", __func__);
		return CSP_ERR_INVAL;
	}

	if (uart_rx_thread_idx >= CONFIG_CSP_UART_RX_THREAD_NUM) {
		LOG_ERR("%s: [%s] No more RX thread can be created. (MAX: %d) Please check CONFIG_CSP_CAN_RX_THREAD_NUM.", __func__, conf->device, CONFIG_CSP_UART_RX_THREAD_NUM);
		return CSP_ERR_DRIVER;
	}

	/* TODO: remove dynamic memory allocation */
	usart_context_t * ctx = k_calloc(1, sizeof(usart_context_t));
	if (ctx == NULL) {
		LOG_ERR("%s: [%s] Failed to allocate %zu bytes from the system heap\n", __func__, conf->device, sizeof(usart_context_t));
		return CSP_ERR_NOMEM;
	}

	strcpy(ctx->name, conf->device);
	ctx->rx_callback = rx_callback;
	ctx->user_data = user_data;
	ctx->fd = device_get_binding(conf->device);
	ctx->cbuf_len = 0;

	if (!device_is_ready(ctx->fd)) {
		LOG_ERR("%s: [%s] Failed to bind UART device\n", __func__, conf->device);
		return CSP_ERR_DRIVER;
	}

	LOG_DBG("device [%s] is ready", conf->device);

	/* Create receive thread */
	k_tid_t rx_tid = k_thread_create(&ctx->rx_thread, uart_rx_stack[uart_rx_thread_idx],
									 K_THREAD_STACK_SIZEOF(uart_rx_stack[uart_rx_thread_idx]),
									 (k_thread_entry_t)csp_usart_rx_thread,
									 ctx, NULL, NULL,
									 CONFIG_CSP_UART_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!rx_tid) {
		LOG_ERR("%s: [%s] k_thread_create() failed", __func__, conf->device);
		return CSP_ERR_DRIVER;
	}
	uart_rx_thread_idx++;

	*return_fd = ctx->fd;

	return CSP_ERR_NONE;
}
