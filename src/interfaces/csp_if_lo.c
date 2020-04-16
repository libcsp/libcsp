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

#include <csp/interfaces/csp_if_lo.h>

#include "../csp_init.h"

/**
 * Loopback interface transmit function
 * @param packet Packet to transmit
 * @return 1 if packet was successfully transmitted, 0 on error
 */
static int csp_lo_tx(const csp_route_t * ifroute, csp_packet_t * packet) {

	/* Drop packet silently if not destined for us. This allows
	 * blackhole routing addresses by setting their nexthop to
	 * the loopback interface.
	 */
	if (packet->id.dst != csp_conf.address) {
		/* Consume and drop packet */
		csp_buffer_free(packet);
		return CSP_ERR_NONE;
	}

	/* Send back into CSP, notice calling from task so last argument must be NULL! */
	csp_qfifo_write(packet, &csp_if_lo, NULL);

	return CSP_ERR_NONE;

}

/* Interface definition */
csp_iface_t csp_if_lo = {
	.name = CSP_IF_LOOPBACK_NAME,
	.nexthop = csp_lo_tx,
};
