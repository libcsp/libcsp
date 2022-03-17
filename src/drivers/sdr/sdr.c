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
#include "rfModeWrapper.h"
#include "error_correctionWrapper.h"
#include "os_timer.h"

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

/* Unfortunately, FreeRTOS doesn't support a timer callback arg, so we have to
 * use static data.
 */
static sdr_context_t *tx_timer_ctx;

/** The timer paces the low-level transmits based on the baudrate */
TimerHandle_t tx_timer;
SemaphoreHandle_t tx_timer_sem;

static void tx_timer_cb( TimerHandle_t xTimer );

/* From the EnduroSat manual, these delays assume the UART speed is 19.2KBaud */
static int sdr_uhf_baud_rate_delay[] = {
    [SDR_UHF_1200_BAUD] = 920,
    [SDR_UHF_2400_BAUD] = 460,
    [SDR_UHF_4800_BAUD] = 240,
    [SDR_UHF_9600_BAUD] = 120,
    [SDR_UHF_19200_BAUD] = 60
};

static void sdr_tx_thread(void *arg) {
    sdr_context_t *ctx = (sdr_context_t *) arg;
    csp_sdr_interface_data_t *ifdata = &ctx->ifdata;

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
            if (fec_csp_to_mpdu(NULL, packet, ifdata->mtu)) {
                if (xTimerStart(tx_timer, 0) != pdPASS) {
                    ex2_log("could not start timer");
                    tx_timer_cb(tx_timer);
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

static void tx_timer_task(void *arg) {
    uint8_t *buf;
    sdr_context_t *ctx = (sdr_context_t *) arg;

    while (1) {
        xSemaphoreTake(tx_timer_sem, portMAX_DELAY);
        int mtu = fec_get_next_mpdu(&buf);
        if (mtu == 0) {
            xTimerStop(tx_timer, 0);
        }
        else {
            (ctx->ifdata.tx_mac)(ctx->ifdata.mac_data, buf, mtu);
        }
    }
}

static void tx_timer_cb( TimerHandle_t xTimer ) {
    /* Warning: although the FreeRTOS docs say we're running this callback in a
     * timer thread, it seems like any blocking call will mess things up.
     */
    xSemaphoreGive(tx_timer_sem);
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
            if (state) {
                ex2_log("%s Rx: received a packet, csp length %d", iface->name, packet->length);
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
    ifdata->baudrate = conf->baudrate;
    ctx->iface.interface_data = &ctx->ifdata;
    ctx->iface.driver_data = ctx;

    int res = csp_sdr_add_interface(&ctx->iface);
    if (res != CSP_ERR_NONE) {
        csp_free(ctx);
        return res;
    }

    tx_timer_sem = xSemaphoreCreateBinary();
    xTaskCreate(tx_timer_task, "sdr_time_tx", TX_TASK_SIZE, ctx, configMAX_PRIORITIES - 1, NULL);

    ifdata->tx_queue = xQueueCreate((unsigned portBASE_TYPE) SDR_TX_QUEUE_SIZE,
                                        (unsigned portBASE_TYPE)sizeof(void*));
    ifdata->tx_sema = xSemaphoreCreateBinary();

    int timer_period = sdr_uhf_baud_rate_delay[conf->baudrate]/portTICK_PERIOD_MS;
    tx_timer = xTimerCreate("UHF-pacer", timer_period, pdTRUE, (void *) 0, tx_timer_cb);
    xTaskCreate(sdr_tx_thread, "sdr_tx", SDR_STACK_SIZE, ctx, 0, NULL);

    ifdata->rx_queue = xQueueCreate((unsigned portBASE_TYPE) SDR_RX_QUEUE_SIZE,
                                         (unsigned portBASE_TYPE) conf->mtu);
    xTaskCreate(sdr_rx_thread, "sdr_rx", SDR_STACK_SIZE, ctx, 0, NULL);

    if (return_iface) {
        *return_iface = &ctx->iface;
    }

    fec_create(RF_MODE_3, NO_FEC);

    return CSP_ERR_NONE;
}
