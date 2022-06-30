/*
 * Copyright (C) 2021  University of Alberta
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/**
 * @file sdr.c
 * @author Ron Unrau
 * @date 2021-10-07
 */

#include <string.h>
#include <csp/csp.h>
#include <csp/arch/csp_malloc.h>
#include <csp/drivers/usart.h>
#include <sdr_driver.h>

int sdr_uart_driver_init(sdr_interface_data_t *ifdata) {
    csp_usart_conf_t *uart_conf = csp_calloc(1, sizeof(csp_usart_conf_t));
    if (!uart_conf)
        return CSP_ERR_NOMEM;

    sdr_uhf_conf_t *uhf_conf = &(ifdata->sdr_conf->uhf_conf);
    uart_conf->device = uhf_conf->device_file;
    uart_conf->baudrate = uhf_conf->uart_baudrate;
    uart_conf->databits = 8;
    uart_conf->stopbits = 2;

    csp_usart_fd_t return_fd;
    int res = csp_usart_open(uart_conf, sdr_rx_isr, ifdata, &return_fd);
    if (res != CSP_ERR_NONE) {
        csp_free(uart_conf);
        return res;
    }

    ifdata->fd = (uintptr_t) return_fd;
    ifdata->tx_func = (sdr_tx_t) csp_usart_write;

    return CSP_ERR_NONE;
}
