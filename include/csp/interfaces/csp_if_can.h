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

#ifndef _CSP_IF_CAN_H_
#define _CSP_IF_CAN_H_

#include <csp/csp.h>

/** AVR8 config struct */
struct can_avr8_conf {
	uint32_t bitrate;
};

/** SocketCAN config struct */
struct can_socketcan_conf {
	char * ifc;
};

/** ARM7TDMI config struct */
struct can_arm7tdmi_conf {
	uint32_t bitrate;
};

int csp_can_tx(csp_id_t cspid, csp_packet_t * packet, unsigned int timeout);
int csp_can_init(uint8_t myaddr, uint8_t promisc, void * conf, int conflen);

#endif /* _CSP_IF_CAN_H_ */
