/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 Gomspace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk) 

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

#ifndef _CSP_INTERFACE_H_
#define _CSP_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <csp/csp.h>

/**
 * Inputs a new packet into the system
 * This function is called from interface drivers ISR to route and accept packets.
 * But it can also be called from a task, provided that the pxTaskWoken parameter is NULL!
 *
 * EXTREMELY IMPORTANT:
 * pxTaskWoken arg must ALWAYS be NULL if called from task,
 * and ALWAYS be NON NULL if called from ISR!
 * If this condition is met, this call is completely thread-safe
 *
 * This function is fire and forget, it returns void, meaning
 * that a packet will always be either accepted or dropped
 * so the memory will always be freed.
 *
 * @param packet A pointer to the incoming packet
 * @param interface A pointer to the incoming interface TX function.
 * @param pxTaskWoken This must be a pointer a valid variable if called from ISR or NULL otherwise!
 */
void csp_qfifo_write(csp_packet_t *packet, csp_iface_t *interface, CSP_BASE_TYPE *pxTaskWoken);

/**
 * csp_new_packet is deprecated, use csp_qfifo_write
 */
#define csp_new_packet csp_qfifo_write

/**
 * Get MAC layer address of next hop.
 * @param node Next hop node
 * @return MAC layer address
 */
uint8_t csp_route_get_mac(uint8_t node);

/**
 * Register your interface in the router core using this function.
 * This must be done in the interface init() function.
 */
void csp_iflist_add(csp_iface_t * ifc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_INTERFACE_H_
