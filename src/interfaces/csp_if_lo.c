/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
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

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_platform.h>
#include <csp/csp_interface.h>
#include <csp/interfaces/csp_if_lo.h>

#include <csp/arch/csp_semaphore.h>
#include <csp/arch/csp_queue.h>

#include "../csp_route.h"

/**
 * Loopback interface transmit function
 * @param packet Packet to transmit
 * @param timeout Timout in ms
 * @return 1 if packet was successfully transmitted, 0 on error
 */
int csp_lo_tx(csp_iface_t * interface, csp_packet_t * packet, uint32_t timeout) {

	/* Send back into CSP, notice calling from task so last argument must be NULL! */
	csp_new_packet(packet, &csp_if_lo, NULL);

	return CSP_ERR_NONE;

}

/* Interface definition */
csp_iface_t csp_if_lo = {
	.name = "LOOP",
	.nexthop = csp_lo_tx,
};
