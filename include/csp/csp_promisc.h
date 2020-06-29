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

#ifndef _CSP_CSP_PROMISC_H_
#define _CSP_CSP_PROMISC_H_

/**
   #file

   Promiscuous packet queue.

   This function is used to enable promiscuous mode for incoming packets, e.g. router, bridge.
   If enabled, a copy of all incoming packets are cloned (using csp_buffer_clone()) and placed in a
   FIFO queue, that can be read using csp_promisc_read().
*/

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Enable promiscuous packet queue.
   @param[in] queue_size Size (max length) of queue for incoming packets.
   @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_promisc_enable(unsigned int queue_size);

/**
   Disable promiscuous mode.
*/
void csp_promisc_disable(void);

/**
   Get/dequeue packet from promiscuous packet queue.

   Returns the first packet from the promiscuous packet queue.
   @param[in] timeout Timeout in ms to wait for a packet.
   @return Packet (free with csp_buffer_free() or re-use packet), NULL on error or timeout.
*/
csp_packet_t *csp_promisc_read(uint32_t timeout);

#ifdef __cplusplus
}
#endif
#endif
