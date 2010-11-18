/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk)

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

#ifndef _CAN_H_
#define _CAN_H_

#include <stdint.h>

#include <csp/csp.h>

/* The can_frame_t and can_id_t types intentionally matches the
 * can_frame struct and can_id types in include/linux/can.h
 */

/** CAN Identifier */
typedef uint32_t can_id_t;

/** CAN Frame */
typedef struct {
	/** 32 bit CAN identifier */
	can_id_t id;
	/** Data Length Code */
	uint8_t dlc;
	/**< Frame Data - 0 to 8 bytes */
	uint8_t data[8] __attribute__((aligned(8)));
} can_frame_t;

/** TX Callback function prototype */
typedef int (*can_tx_callback_t)(can_id_t id, CSP_BASE_TYPE * task_woken);

/** RX Callback function prototype */
typedef int (*can_rx_callback_t)(can_frame_t * frame, CSP_BASE_TYPE * task_woken);

int can_init(uint32_t id, uint32_t mask, can_tx_callback_t txcb, can_rx_callback_t rxcb, void * conf, int conflen);
int can_mbox_init(void);
int can_mbox_get(void);
int can_mbox_release(int mbox);
int can_mbox_data(int mbox, can_id_t id, uint8_t * data, uint8_t dlc);
int can_mbox_send(int mbox);

#endif /* _CAN_H_ */
