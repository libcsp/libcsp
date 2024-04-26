

#include <csp/interfaces/csp_if_kiss.h>

#include <csp/csp_debug.h>
#include <stdlib.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>

typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;
	csp_kiss_interface_data_t ifdata;
	csp_usart_fd_t fd;
} kiss_context_t;

static int kiss_driver_tx(void * driver_data, const unsigned char * data, size_t data_length) {

	kiss_context_t * ctx = driver_data;
	if (csp_usart_write(ctx->fd, data, data_length) == (int)data_length) {
		return CSP_ERR_NONE;
	}
	return CSP_ERR_TX;
}

static void kiss_driver_rx(void * user_data, uint8_t * data, size_t data_size, void * pxTaskWoken) {

	kiss_context_t * ctx = user_data;
	csp_kiss_rx(&ctx->iface, data, data_size, pxTaskWoken);
}

int csp_usart_open_and_add_kiss_interface(const csp_usart_conf_t * conf, const char * ifname, uint16_t addr, csp_iface_t ** return_iface) {

	if (ifname == NULL) {
		ifname = CSP_IF_KISS_DEFAULT_NAME;
	}

	kiss_context_t * ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		return CSP_ERR_NOMEM;
	}

	strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
	ctx->iface.name = ctx->name;
	ctx->iface.addr = addr;
	ctx->iface.driver_data = ctx;
	ctx->iface.interface_data = &ctx->ifdata;
	ctx->ifdata.tx_func = kiss_driver_tx;

#if (CSP_ZEPHYR)
	ctx->fd = NULL;
#else
	ctx->fd = -1;
#endif

	int res = csp_kiss_add_interface(&ctx->iface);
	if (res == CSP_ERR_NONE) {
		res = csp_usart_open(conf, kiss_driver_rx, ctx, &ctx->fd);
	}

	if (return_iface) {
		*return_iface = &ctx->iface;
	}

	return res;
}
