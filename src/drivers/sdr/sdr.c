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
#include <csp/csp_endian.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/drivers/sdr.h>
#include "fec.h"
#include "rfModeWrapper.h"
#include "error_correctionWrapper.h"

#ifdef CSP_POSIX
#include <stdio.h>
#define ex2_log printf
#endif // CSP_POSIX

typedef struct {
    char name[CSP_IFLIST_NAME_MAX + 1];
    csp_iface_t iface;
    csp_sdr_interface_data_t ifdata;
} sdr_context_t;

#define SDR_TX_QUEUE_SIZE 32
#define SDR_RX_QUEUE_SIZE 8
#define MPDU_FIFO_SIZE 16

#define TX_TASK_SIZE 512
#define SDR_STACK_SIZE 512

/* From the EnduroSat manual, these delays assume the UART speed is 19.2KBaud */
static int sdr_uhf_baud_rate_delay[] = {
    [SDR_UHF_1200_BAUD] = 920,
    [SDR_UHF_2400_BAUD] = 460,
    [SDR_UHF_4800_BAUD] = 240,
    [SDR_UHF_9600_BAUD] = 120,
    [SDR_UHF_19200_BAUD] = 60
};

static void *sdr_tx_thread(void *arg) {
    sdr_context_t *ctx = (sdr_context_t *) arg;
    csp_sdr_interface_data_t *ifdata = &ctx->ifdata;
  
    while (1) {
        csp_packet_t *packet = 0;
        if (csp_queue_dequeue(ifdata->tx_queue, &packet, CSP_MAX_DELAY) != true) {
            ex2_log("queue receive failed");
            csp_bin_sem_post(&(ifdata->tx_sema));
            continue;
        }

        uint16_t plen = csp_ntoh16(packet->length);
        //idout.ext = csp_ntoh32(packet->id.ext);
        if (ifdata->config_flags & SDR_CONF_FEC) {
            if (fec_csp_to_mpdu(packet, ifdata->mtu)) {
                uint8_t *buf;
                int delay_time = sdr_uhf_baud_rate_delay[ifdata->baudrate];
                int mtu = fec_get_next_mpdu((void **)&buf);
                while (mtu != 0) {
                    #ifdef CSP_POSIX
                    (ctx->ifdata.tx_mac)((long)ctx->ifdata.mac_data, buf, mtu);
                    #endif // CSP_POSIX
                    #ifdef CSP_FREERTOS
                    (ctx->ifdata.tx_mac)((int)ctx->ifdata.mac_data, buf, mtu);
                    #endif // CSP_FREERTOS
                    mtu = fec_get_next_mpdu((void **)&buf);
                    csp_sleep_ms(delay_time);
                }
                
                /*if (xTimerStart(tx_timer, 0) != true) {
                    ex2_log("could not start timer");
                    tx_timer_cb(tx_timer);
                }*/
            }
        }
        else {
            /* Without FEC, just write the entire CSP packet */
            #ifdef CSP_POSIX
            (ifdata->tx_mac)((long)ifdata->mac_data, (const uint8_t *) packet, plen);
            #endif // CSP_POSIX
            #ifdef CSP_FREERTOS
            (ifdata->tx_mac)((int)ifdata->mac_data, (const uint8_t *) packet, plen);
            #endif // CSP_FREERTOS
        }
        /* Unblock the sender so it can free the packet */
        csp_bin_sem_post(&(ifdata->tx_sema));
    }
    return 0; // so CSP stops complaining
}

static void *sdr_rx_thread(void *arg) {
    static uint8_t data[SDR_UHF_MAX_MTU];
    sdr_context_t *ctx = (sdr_context_t *) arg;
    csp_iface_t *iface = &ctx->iface;
    csp_sdr_interface_data_t *ifdata = &ctx->ifdata;
    const csp_conf_t *conf = csp_get_conf();
    /* State machine state */
    csp_packet_t *packet = 0;
    uint8_t id = 0;

    while (1) {
        if (csp_queue_dequeue(ifdata->rx_queue, data, CSP_MAX_TIMEOUT) != true) {
            ex2_log("SDR loopback: queue receive failed");
            continue;
        }

        if (ifdata->config_flags & SDR_CONF_FEC) {
            fec_state_t state = fec_mpdu_to_csp(data, &packet, &id, ifdata->mtu);
            if (state) {
                ex2_log("%s Rx: received a packet, csp length %d", iface->name, csp_ntoh16(packet->length));
                if (strcmp(iface->name, CSP_IF_SDR_LOOPBACK_NAME) == 0) {
                    /* This is an unfortunate hack to be able to do loopback.
                     * We have to change the outgoing packet destination (the
                     * device) to the incoming destination (us) or else CSP will
                     * drop the packet.
                     */
                    packet->id.dst = conf->address;
                }
                csp_qfifo_write(packet, iface, NULL);
                //csp_buffer_free(packet);
            }

        }
        else {
            csp_packet_t *incoming = (csp_packet_t *) data;
            uint16_t plen = csp_ntoh16(incoming->length);
            packet = csp_buffer_get(plen);
            if (!packet) {
                ex2_log("no more CSP packets");
                break;
            }
            packet->length = incoming->length;
            ex2_log("%s Rx: received a packet, csp length %d", iface->name, plen);

            memcpy(packet->data, incoming->data, plen);
            csp_qfifo_write(packet, iface, NULL);
        }
    }
    return 0; // so CSP stops complaining
}

int csp_sdr_open_and_add_interface(const csp_sdr_conf_t *conf, const char *ifname, csp_iface_t **return_iface) {

    if (conf->baudrate < 0 || conf->baudrate >= SDR_UHF_END_BAUD) {
        return CSP_ERR_INVAL;
    }
    sdr_context_t *ctx = csp_calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return CSP_ERR_NOMEM;
    }

    strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
    ctx->iface.name = ctx->name;
    csp_sdr_interface_data_t *ifdata = &ctx->ifdata;

    if (conf->use_fec)
        ifdata->config_flags = SDR_CONF_FEC;
    ifdata->mtu = conf->mtu;
    ifdata->baudrate = conf->uart_baudrate;
    ctx->iface.interface_data = &ctx->ifdata;
    ctx->iface.driver_data = ctx;

    int res = csp_sdr_add_interface(&ctx->iface);
    if (res != CSP_ERR_NONE) {
        csp_free(ctx);
        return res;
    }

    ifdata->tx_queue = csp_queue_create((unsigned CSP_BASE_TYPE) SDR_TX_QUEUE_SIZE,
                                        (unsigned CSP_BASE_TYPE)sizeof(void*));
    csp_bin_sem_create(&(ifdata->tx_sema));

    csp_thread_create((csp_thread_func_t)sdr_tx_thread, "sdr_tx", SDR_STACK_SIZE, ctx, 0, NULL);

    ifdata->rx_queue = csp_queue_create((unsigned CSP_BASE_TYPE) SDR_RX_QUEUE_SIZE,
                                         (unsigned CSP_BASE_TYPE) conf->mtu);
    csp_thread_create((csp_thread_func_t)sdr_rx_thread, "sdr_rx", SDR_STACK_SIZE, ctx, 0, NULL);

    if (return_iface) {
        *return_iface = &ctx->iface;
    }

    fec_create(RF_MODE_3, NO_FEC);

    return CSP_ERR_NONE;
}
