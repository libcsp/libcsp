/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk) 

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

#ifndef _CSP_IF_CAN_H_
#define _CSP_IF_CAN_H_

#include <stdint.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>

/** CAN interface modes */
#define CSP_CAN_MASKED		0
#define CSP_CAN_PROMISC		1

extern csp_iface_t csp_if_can;

/** AT90CAN128 config struct */
struct can_at90can128_conf {
	uint32_t bitrate;
	uint32_t clock_speed;
};

/** SocketCAN config struct */
struct can_socketcan_conf {
	char * ifc;
};

/** AT91SAM7A1 config struct */
struct can_at91sam7a1_conf {
	uint32_t bitrate;
	uint32_t clock_speed;
};

/** AT91SAM7A3 config struct */
struct can_at91sam7a3_conf {
	uint32_t bitrate;
	uint32_t clock_speed;
};

/**
 * CAN interface transmit function
 * @param packet Packet to transmit
 * @param timeout Timout in ms
 * @return 1 if packet was successfully transmitted, 0 on error
 */
int csp_can_tx(csp_packet_t * packet, uint32_t timeout);

/**
 * Init CAN interface
 * @param mode Must be either CSP_CAN_MASKED or CSP_CAN_PROMISC
 * @param conf Pointer to configuration struct. This is subsystem dependent
 * @param conflen Size of configuration struct
 * @return 0 if CAN interface was successfully initialized, -1 otherwise
 */
int csp_can_init(uint8_t mode, void * conf, int conflen);

#endif /* _CSP_IF_CAN_H_ */
