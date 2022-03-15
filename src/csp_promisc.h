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

#ifndef _SRC_CSP_PROMISC_H_
#define _SRC_CSP_PROMISC_H_

#include <csp/csp_promisc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add packet to promiscuous mode packet queue
 * @param packet Packet to add to the queue
 */
void csp_promisc_add(csp_packet_t * packet);

#ifdef __cplusplus
}
#endif
#endif
