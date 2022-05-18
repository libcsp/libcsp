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
 * @file service_utilities.c
 * @author Andrew Rooney
 * @date 2020-07-23
 */
#include <FreeRTOS.h>
#include <os_semphr.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_thread.h>

#include "HL_sci.h"
#include "HL_sys_common.h"
#include "HL_system.h"
#include "system.h"

#define CSP_TX_TIMEOUT_MS 1000

typedef struct {
    csp_usart_callback_t rx_callback;
    void * user_data;
    sciBASE_t *fd;
    xTaskHandle rx_thread;
} usart_context_t;

static xQueueHandle sciData;
static uint8_t incomingData;
static bool uhf_command_mode = false;

SemaphoreHandle_t tx_semphr;

void usart_rx_thread(void * arg) {

    usart_context_t * ctx = arg;
    uint8_t rxByte;

    // Receive loop
    while (1) {
        if (xQueueReceive(sciData, &rxByte, portMAX_DELAY)){
            if(!uhf_command_mode){
                ctx->rx_callback(ctx->user_data, &rxByte, sizeof(uint8_t), NULL);
            }else{
                uhf_command_mode_callback(rxByte);
            }

        }
    }
}

void uhf_enter_direct_command_mode(){
    uhf_command_mode = true;
}

void uhf_exit_direct_command_mode(){
    uhf_command_mode = false;
}

int csp_usart_write(csp_usart_fd_t fd, const void * data, size_t data_length) {
    sciSend(CSP_SCI, data_length, (uint8_t*)data);
    if (xSemaphoreTake(tx_semphr, CSP_TX_TIMEOUT_MS) != pdTRUE) {
        return CSP_ERR_TIMEDOUT;
    }
    return CSP_ERR_NONE;
}

int csp_usart_open(const csp_usart_conf_t *conf, csp_usart_callback_t rx_callback, void * user_data,  csp_usart_fd_t* return_fd) {
    sciSetBaudrate(CSP_SCI, conf->baudrate);

    usart_context_t * ctx = csp_calloc(1, sizeof(*ctx));

    ctx->rx_callback = rx_callback;
    ctx->user_data = user_data;
    ctx->fd = CSP_SCI;
    return_fd = CSP_SCI;
    sciData = xQueueCreate((unsigned portBASE_TYPE)32,
            (unsigned portBASE_TYPE)sizeof(uint8_t));
    tx_semphr = xSemaphoreCreateBinary();
    xTaskCreate(usart_rx_thread, "usart_rx", 256, (void *) ctx, configMAX_PRIORITIES, &ctx->rx_thread);

    sciReceive(ctx->fd, sizeof(uint8_t), &incomingData);

    return CSP_ERR_NONE;
}


void csp_sciNotification(sciBASE_t *sci, unsigned flags) {
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;



    switch (flags) {
    case SCI_RX_INT:
        xQueueSendToBackFromISR( sciData, &incomingData, &xHigherPriorityTaskWoken );
        sciReceive(sci, sizeof(uint8_t), &incomingData);
#ifndef UHF_IS_STUBBED
        if(!uhf_command_mode) uhf_pipe_timer_reset_from_isr(&xHigherPriorityTaskWoken, incomingData);
#endif
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        break;
    case SCI_TX_INT:
        xSemaphoreGiveFromISR(tx_semphr, &xHigherPriorityTaskWoken);
#ifndef UHF_IS_STUBBED
        if(!uhf_command_mode) uhf_pipe_timer_reset_from_isr(&xHigherPriorityTaskWoken, NULL);
#endif
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        break;
    }
}

void esmGroup1Notification(int but) {
    return;
}

void esmGroup2Notification(int but) {
    return;
}
