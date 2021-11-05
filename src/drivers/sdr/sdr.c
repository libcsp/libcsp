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
#include <csp/arch/csp_malloc.h>
#include <csp/drivers/sdr.h>
#include "os_queue.h"
#include "os_task.h"
#include "util/service_utilities.h"
#include "circular_buffer.h"
#include "fec.h"

typedef struct {
    char name[CSP_IFLIST_NAME_MAX + 1];
    csp_iface_t iface;
    csp_sdr_interface_data_t ifdata;
} sdr_context_t;

#define SDR_TX_QUEUE_SIZE 32
#define SDR_RX_QUEUE_SIZE 8
#define MPDU_FIFO_SIZE 16

#define SDR_STACK_SIZE 256

/* Unfortunately, FreeRTOS doesn't support a timer callback arg, so we have to
 * use static data.
 */
static CircularBufferHandle mpdu_fifo;
static sdr_context_t *tx_timer_ctx;

static void tx_timer_cb( TimerHandle_t xTimer );

static void sdr_tx_thread(void *arg) {
    sdr_context_t *ctx = (sdr_context_t *) arg;
    csp_sdr_interface_data_t *ifdata = &ctx->ifdata;

    mpdu_fifo = CircularBufferCreate(ifdata->mtu, MPDU_FIFO_SIZE);
    if (!mpdu_fifo) {
        ex2_log("can't create MPDU FIFO");
        return;
    }
    tx_timer_ctx = ctx;
  
    while (1) {
        QueueHandle_t txq = ifdata->tx_queue;
        csp_packet_t *packet = 0;
        if (xQueueReceive(txq, &packet, portMAX_DELAY) != pdPASS) {
            ex2_log("queue receive failed");
            xSemaphoreGive(ifdata->tx_sema);
            continue;
        }

        ex2_log("uhf Tx thread got a packet: len %d, data: %p", packet->length, (char *) packet->data);
        if (ifdata->config_flags & SDR_CONF_FEC) {
            if (fec_csp_to_mpdu(mpdu_fifo, packet, ifdata->mtu)) {
                if (xTimerStart(ctx->ifdata.tx_timer, 0) != pdPASS) {
                    ex2_log("could not start timer");
                    tx_timer_cb(ctx->ifdata.tx_timer);
                }
            }
        }
        else {
            /* Without FEC, just write the entire CSP packet */
            (ifdata->tx_mac)(ifdata->mac_data, (const uint8_t *) packet, packet->length);
        }
        /* Unblock the sender so it can free the packet */
        xSemaphoreGive(ifdata->tx_sema);
    }
}

static void tx_timer_cb( TimerHandle_t xTimer ) {
    /* Warning: although the FreeRTOS docs say we're running this callback in a
     * timer thread, it seems like any blocking call will mess things up.
     */
    void *tail = CircularBufferNextTail(mpdu_fifo);
    if (tail == 0) {
        xTimerStop(xTimer, 0);
    }
    else {
        (tx_timer_ctx->ifdata.tx_mac)(tx_timer_ctx->ifdata.mac_data, tail, tx_timer_ctx->ifdata.mtu);
        CircularBufferReceive(mpdu_fifo);
    }
}

static void sdr_rx_thread(void *arg) {
    static uint8_t data[SDR_UHF_MAX_MTU];
    sdr_context_t *ctx = (sdr_context_t *) arg;
    csp_iface_t *iface = &ctx->iface;
    csp_sdr_interface_data_t *ifdata = &ctx->ifdata;
    const csp_conf_t *conf = csp_get_conf();
    /* State machine state */
    csp_packet_t *packet = 0;
    uint8_t id = 0;

    while (1) {
        if (xQueueReceive(ifdata->rx_queue, data, portMAX_DELAY) != pdPASS) {
            ex2_log("SDR loopback: queue receive failed");
            continue;
        }

        if (ifdata->config_flags & SDR_CONF_FEC) {
            fec_state_t state = fec_mpdu_to_csp(data, (uint8_t **) &packet, &id, ifdata->mtu);
            switch(state) {
            case FEC_STATE_IN_PROGRESS:
                break;
            case FEC_STATE_COMPLETE:
                ex2_log("%s Rx: received a packet, csp id %d length %d", iface->name, packet->id, packet->length);
                if (strcmp(iface->name, CSP_IF_SDR_LOOPBACK_NAME) == 0) {
                    /* This is an unfortunate hack to be able to do loopback.
                     * We have to change the outgoing packet destination (the 
                     * device) to the incoming destination (us) or else CSP will
                     * drop the packet.
                     */
                    packet->id.dst = conf->address;
                }
                csp_qfifo_write(packet, iface, NULL);
                /* csp_qfifo_write disposes the packet for us. */
                packet = 0;
                break;
            case FEC_STATE_INCOMPLETE:
                /* oops! missed a MPDU! Finish of the CSP packet and try again. */
                if (strcmp(iface->name, CSP_IF_SDR_LOOPBACK_NAME) == 0) {
                    packet->id.dst = conf->address; // same hack as above
                }
                csp_qfifo_write(packet, iface, NULL);
                packet = 0;
                state = fec_mpdu_to_csp(data, (uint8_t **) &packet, &id, ifdata->mtu);
                if (state != FEC_STATE_IN_PROGRESS) {
                    ex2_log("off the rails :-(");
                }
                break;
            default:
                ex2_log("corrupt packet");
                break;
            }
        }
        else {
            csp_packet_t *incoming = (csp_packet_t *) data;
            packet = csp_buffer_get(incoming->length);
            if (!packet) {
                ex2_log("no more CSP packets");
                break;
            }
            packet->length = incoming->length;
            ex2_log("%s Rx: received a packet, csp length %d", iface->name, packet->length);

            memcpy(packet->data, incoming->data, incoming->length);
            csp_qfifo_write(packet, iface, NULL);
        }
    }
}

int csp_sdr_open_and_add_interface(const csp_sdr_conf_t *conf, const char *ifname, csp_iface_t **return_iface) {
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
    ifdata->baudrate = conf->baudrate;
    ctx->iface.interface_data = &ctx->ifdata;
    ctx->iface.driver_data = ctx;

    int res = csp_sdr_add_interface(&ctx->iface);
    if (res != CSP_ERR_NONE) {
        csp_free(ctx);
        return res;
    }

    ifdata->tx_queue = xQueueCreate((unsigned portBASE_TYPE) SDR_TX_QUEUE_SIZE,
                                        (unsigned portBASE_TYPE)sizeof(void*));
    ifdata->tx_sema = xSemaphoreCreateBinary();
    ifdata->tx_timer = xTimerCreate("UHF-pacer", 100, pdTRUE, (void *) 0, tx_timer_cb);
    xTaskCreate(sdr_tx_thread, "sdr_tx", SDR_STACK_SIZE, ctx, 0, NULL);

    ifdata->rx_queue = xQueueCreate((unsigned portBASE_TYPE) SDR_RX_QUEUE_SIZE,
                                         (unsigned portBASE_TYPE) conf->mtu);
    xTaskCreate(sdr_rx_thread, "sdr_rx", SDR_STACK_SIZE, ctx, 0, NULL);

    if (return_iface) {
        *return_iface = &ctx->iface;
    }

    return CSP_ERR_NONE;
}
