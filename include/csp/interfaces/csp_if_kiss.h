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

#ifndef CSP_IF_KISS_H_
#define CSP_IF_KISS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>

extern csp_iface_t csp_if_kiss;

/**
 * Setup KISS callback handler into the USART with specific handle number
 * @param handle USART number to use
 * @return CSP_ERR_NONE always
 */
int csp_kiss_init(int handle);

/**
 * CAN interface transmit function
 * @param packet Packet to transmit
 * @param timeout Timout in ms
 * @return 1 if packet was successfully transmitted, 0 on error
 */
int csp_kiss_tx(csp_packet_t * packet, uint32_t timeout);

/**
 * When a frame is received, decode the kiss-stuff
 * and eventually send it directly to the CSP new packet function.
 */
void csp_kiss_rx(uint8_t * buf, int len, void * pxTaskWoken);

#endif /* CSP_IF_KISS_H_ */
