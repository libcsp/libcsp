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
 * @file can.c
 * @author Andrew Rooney
 * @date 2021-02-17
 */

#if (CSP_FREERTOS)
#include "csp/interfaces/csp_if_can.h"
#include <csp/csp_platform.h>
#include <string.h>
#include "os_queue.h"
#include "os_task.h"
#include "HL_sys_core.h"
#include "HL_can.h"

#define CAN_COUNTER_MAX 10000

typedef struct {
    char name[CSP_IFLIST_NAME_MAX + 1];
    csp_iface_t iface;
    csp_can_interface_data_t ifdata;
} can_context_t;

can_context_t * ctx;
static xQueueHandle canData;

typedef struct can_frame {
    uint32_t id;
    uint32_t dlc;
    uint8_t data[8];
} can_frame_t;

void can_rx_thread(void * arg) {
    can_context_t * ctx = arg;
    can_frame_t rxFrame;
    while (1) {
        if (xQueueReceive(canData, &rxFrame, portMAX_DELAY)){
            csp_can_rx(&ctx->iface, rxFrame.id, rxFrame.data, rxFrame.dlc, NULL);
        }
    }
}

static int csp_can_tx_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc)
{
    if (dlc > 8) {
        return CSP_ERR_INVAL;
    }
    id = id | 0b01100000000000000000000000000000;
    canBASE_t *canREG = canREG1;
    uint16_t can_counter = 0;
    switch (dlc) {
    case 8:
        while(canIsTxMessagePending(canREG, canMESSAGE_BOX1)){
            can_counter++;
            if(can_counter > CAN_COUNTER_MAX){
                return CSP_ERR_TIMEDOUT;
            }
        }
        canUpdateID(canREG, canMESSAGE_BOX1, id);
        uint32_t x = canGetID(canREG, canMESSAGE_BOX1);
        canTransmit(canREG, canMESSAGE_BOX1, data);
        break;
    case 7:
        while(canIsTxMessagePending(canREG, canMESSAGE_BOX3)){
            can_counter++;
            if(can_counter > CAN_COUNTER_MAX){
                return CSP_ERR_TIMEDOUT;
            }
        }
        canUpdateID(canREG, canMESSAGE_BOX3, id);
        canTransmit(canREG, canMESSAGE_BOX3, data);
        break;
    case 6:
        while(canIsTxMessagePending(canREG, canMESSAGE_BOX5)){
            can_counter++;
            if(can_counter > CAN_COUNTER_MAX){
                return CSP_ERR_TIMEDOUT;
            }
        }
        canUpdateID(canREG, canMESSAGE_BOX5, id);
        canTransmit(canREG, canMESSAGE_BOX5, data);
        break;
    case 5:
        while(canIsTxMessagePending(canREG, canMESSAGE_BOX7)){
            can_counter++;
            if(can_counter > CAN_COUNTER_MAX){
                return CSP_ERR_TIMEDOUT;
            }
        }
        canUpdateID(canREG, canMESSAGE_BOX7, id);
        canTransmit(canREG, canMESSAGE_BOX7, data);
        break;
    case 4:
        while(canIsTxMessagePending(canREG, canMESSAGE_BOX9)){
            can_counter++;
            if(can_counter > CAN_COUNTER_MAX){
                return CSP_ERR_TIMEDOUT;
            }
        }
        canUpdateID(canREG, canMESSAGE_BOX9, id);
        canTransmit(canREG, canMESSAGE_BOX9, data);
        break;
    case 3:
        while(canIsTxMessagePending(canREG, canMESSAGE_BOX11)){
            can_counter++;
            if(can_counter > CAN_COUNTER_MAX){
                return CSP_ERR_TIMEDOUT;
            }
        }
        canUpdateID(canREG, canMESSAGE_BOX11, id);
        canTransmit(canREG, canMESSAGE_BOX11, data);
        break;
    case 2:
        while(canIsTxMessagePending(canREG, canMESSAGE_BOX13)){
            can_counter++;
            if(can_counter > CAN_COUNTER_MAX){
                return CSP_ERR_TIMEDOUT;
            }
        }
        canUpdateID(canREG, canMESSAGE_BOX13, id);
        canTransmit(canREG, canMESSAGE_BOX13, data);
        break;
    case 1:
        while(canIsTxMessagePending(canREG, canMESSAGE_BOX15)){
            can_counter++;
            if(can_counter > CAN_COUNTER_MAX){
                return CSP_ERR_TIMEDOUT;
            }
        }
        canUpdateID(canREG, canMESSAGE_BOX15, id);
        canTransmit(canREG, canMESSAGE_BOX15, data);
        break;
    }

    return CSP_ERR_NONE;
}

int csp_can_open_and_add_interface(const char * ifname, csp_iface_t ** return_iface)
{
    if (ifname == NULL) {
        ifname = CSP_IF_CAN_DEFAULT_NAME;
    }
    can_context_t * ctx = csp_calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return CSP_ERR_NOMEM;
    }

    strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
    ctx->iface.name = ctx->name;
    ctx->iface.interface_data = &ctx->ifdata;
    ctx->iface.driver_data = ctx;
    ctx->ifdata.tx_func = csp_can_tx_frame;
    int res = csp_can_add_interface(&ctx->iface);
    if (res != CSP_ERR_NONE) {
        vPortFree(ctx);
        return res;
    }
    canData = xQueueCreate((unsigned portBASE_TYPE)32,
                (unsigned portBASE_TYPE)sizeof(can_frame_t));
     xTaskCreate(can_rx_thread, "can_rx", 256, (void *) ctx, 0, NULL);
    if (return_iface) {
        *return_iface = &ctx->iface;
    }

    return CSP_ERR_NONE;
}

void canMessageNotification(canBASE_t *node, uint32 messageBox)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    can_frame_t rxFrame;
    uint32_t rxSize;

    rxFrame.id = canGetID(node, messageBox);
    canGetDataAndSize(node, messageBox, rxFrame.data, &rxSize);
    rxFrame.dlc = rxSize;
    xQueueSendToBackFromISR(canData, &rxFrame, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif /* CSP_FREERTOS */
