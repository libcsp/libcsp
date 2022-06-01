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
#include <csp/csp_interface.h>
#include <csp/interfaces/csp_if_sdr.h>
#include <csp/csp_endian.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/drivers/sdr.h>
#include "csp/drivers/fec.h"
#include <csp/drivers/usart.h>

#ifdef CSP_POSIX
#include <stdio.h>
#define ex2_log printf
#endif // CSP_POSIX

#ifdef SDR_UART

#ifdef CSP_FREERTOS
#define RX_TASK_STACK_SIZE 512
#else
#define RX_TASK_STACK_SIZE 4096
#endif

/* From the EnduroSat manual, these delays assume the UART speed is 19.2KBaud */
static int sdr_uhf_baud_rate_delay[] = {
    [SDR_UHF_1200_BAUD] = 920,
    [SDR_UHF_2400_BAUD] = 460,
    [SDR_UHF_4800_BAUD] = 240,
    [SDR_UHF_9600_BAUD] = 120,
    [SDR_UHF_19200_BAUD] = 60
};

int csp_sdr_tx(const csp_route_t *ifroute, csp_packet_t *packet) {
    csp_iface_t *iface = (csp_iface_t *)ifroute->iface;
    csp_sdr_interface_data_t *ifdata = (csp_sdr_interface_data_t *)ifroute->iface->interface_data;
    csp_sdr_conf_t *sdr_conf = (csp_sdr_conf_t *)iface->driver_data;

    if (fec_csp_to_mpdu(ifdata->mac_data, packet, sdr_conf->mtu)) {
        uint8_t *buf;
        int delay_time = sdr_uhf_baud_rate_delay[ifdata->uhf_baudrate];
        size_t mtu = (size_t)fec_get_next_mpdu(ifdata->mac_data, (void **)&buf);
        while (mtu != 0) {
            (ifdata->tx_func)(ifdata->fd, buf, mtu);
            mtu = fec_get_next_mpdu(ifdata->mac_data, (void **)&buf);
            csp_sleep_ms(delay_time);
        }
    }
    return CSP_ERR_NONE;
}

void csp_sdr_rx(void *cb_data, void *buf, size_t len, void *pxTaskWoken) {
    static int total_recv = 0;
    total_recv += len;
    csp_iface_t *iface = (csp_iface_t *) cb_data;
    csp_sdr_conf_t *conf = (csp_sdr_conf_t *)iface->driver_data;
    csp_sdr_interface_data_t *ifdata = (csp_sdr_interface_data_t *)iface->interface_data;
    uint8_t *ptr = buf;
    for (size_t i=0; i<len; i++) {
        ifdata->rx_mpdu[ifdata->rx_mpdu_index] = ptr[i];
        ifdata->rx_mpdu_index++;
        if (ifdata->rx_mpdu_index >= conf->mtu) {
            ifdata->rx_mpdu_index = 0;
            // This is an isr, if this fails there's nothing that can be done
            csp_queue_enqueue(ifdata->rx_queue, ifdata->rx_mpdu, QUEUE_NO_WAIT);
        }
    }
}

CSP_DEFINE_TASK(csp_sdr_rx_task) {
    csp_iface_t *iface = (csp_iface_t *)param;
    csp_sdr_interface_data_t *ifdata = (csp_sdr_interface_data_t *)iface->interface_data;
    csp_sdr_conf_t *sdr_conf = (csp_sdr_conf_t *)iface->driver_data;
    uint8_t *recv_buf;
    recv_buf = csp_malloc(sdr_conf->mtu);
    csp_packet_t *packet = 0;
    const csp_conf_t *conf = csp_get_conf();

    while (1) {
        if (csp_queue_dequeue(ifdata->rx_queue, recv_buf, CSP_MAX_TIMEOUT) != true) {
            continue;
        }

        bool state = fec_mpdu_to_csp(ifdata->mac_data, recv_buf, &packet, sdr_conf->mtu);
        if (state) {
            if (strcmp(iface->name, CSP_IF_SDR_LOOPBACK_NAME) == 0) {
                /* This is an unfortunate hack to be able to do loopback.
                    * We have to change the outgoing packet destination (the
                    * device) to the incoming destination (us) or else CSP will
                    * drop the packet.
                    */
                packet->id.dst = conf->address;
            }
            csp_qfifo_write(packet, iface, NULL);
        }
    }
}


int csp_sdr_driver_init(csp_iface_t *iface) {

    if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
        return CSP_ERR_INVAL;
    }
    csp_sdr_interface_data_t *ifdata = (csp_sdr_interface_data_t *)iface->interface_data;
    ifdata->tx_func = (csp_sdr_driver_tx_t) csp_usart_write;

    csp_usart_conf_t *conf = csp_calloc(1, sizeof(csp_usart_conf_t));
    if (!conf)
        return CSP_ERR_NOMEM;
    csp_sdr_conf_t *sdr_conf = (csp_sdr_conf_t *)iface->driver_data;
    conf->device = sdr_conf->device_file;
    conf->baudrate = sdr_conf->uart_baudrate;
    conf->databits = 8;
    conf->stopbits = 2;

    ifdata->rx_queue = csp_queue_create(2, ((csp_sdr_conf_t *)iface->driver_data)->mtu);
    csp_thread_create(csp_sdr_rx_task, "sdr_rx", RX_TASK_STACK_SIZE, (void *)iface, 0, NULL);

    csp_usart_fd_t return_fd;
    int res = csp_usart_open(conf, (csp_usart_callback_t)csp_sdr_rx, iface, &return_fd);
    ifdata->fd = (CSP_BASE_TYPE) return_fd;
    if (res != CSP_ERR_NONE) {
        csp_free(conf);
        return res;
    }

    #ifndef CSP_POSIX
    else if (strcmp(iface->name, CSP_IF_SDR_LOOPBACK_NAME) == 0) {
        sdr_loopback_open(iface);
    }
    #endif // CSP_POSIX

    return 0;
}

#endif /* SDR_UART */
