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

#include <csp/interfaces/csp_if_kiss.h>

#include <stdio.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/arch/csp_malloc.h>

typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;
	csp_kiss_interface_data_t ifdata;
	csp_usart_fd_t fd;
} kiss_context_t;

static int kiss_driver_tx(void *driver_data, const unsigned char * data, size_t data_length) {

	kiss_context_t * ctx = driver_data;
	if (csp_usart_write(ctx->fd, data, data_length) == (int) data_length) {
		return CSP_ERR_NONE;
	}
	return CSP_ERR_TX;
}

static void kiss_driver_rx(void * user_data, uint8_t * data, size_t data_size, void * pxTaskWoken) {

	kiss_context_t * ctx = user_data;
	csp_kiss_rx(&ctx->iface, data, data_size, NULL);
}

int csp_usart_open_and_add_kiss_interface(const csp_usart_conf_t *conf, const char * ifname, csp_iface_t ** return_iface) {

	if (ifname == NULL) {
		ifname = CSP_IF_KISS_DEFAULT_NAME;
	}

	csp_log_info("INIT %s: device: [%s], bitrate: %d",
			ifname, conf->device, conf->baudrate);

	kiss_context_t * ctx = csp_calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		return CSP_ERR_NOMEM;
	}

	strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
	ctx->iface.name = ctx->name;
	ctx->iface.driver_data = ctx;
	ctx->iface.interface_data = &ctx->ifdata;
	ctx->ifdata.tx_func = kiss_driver_tx;
#if (CSP_WINDOWS)
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
