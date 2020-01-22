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

#include <csp/csp.h>

#include <csp/csp_interface.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include <csp/arch/csp_thread.h>
#include "csp_qfifo.h"
#include "csp_io.h"
#include "csp_promisc.h"

typedef struct {
	csp_iface_t* iface;
} bridge_interface_t;

static bridge_interface_t bif_a;
static bridge_interface_t bif_b;

static CSP_DEFINE_TASK(csp_bridge) {

	/* Here there be bridging */
	while (1) {

		/* Get next packet to route */
		csp_qfifo_t input;
		if (csp_qfifo_read(&input) != CSP_ERR_NONE) {
			continue;
		}

		csp_packet_t * packet = input.packet;

		csp_log_packet("Input: Src %u, Dst %u, Dport %u, Sport %u, Pri %u, Flags 0x%02X, Size %"PRIu16,
				packet->id.src, packet->id.dst, packet->id.dport,
				packet->id.sport, packet->id.pri, packet->id.flags, packet->length);

		/* Here there be promiscuous mode */
#if (CSP_USE_PROMISC)
		csp_promisc_add(packet);
#endif

		/* Find the opposing interface */
		csp_route_t route;
		if (input.iface == bif_a.iface) {
			route.iface = bif_b.iface;
			route.via = CSP_NO_VIA_ADDRESS;
		} else {
			route.iface = bif_a.iface;
			route.via = CSP_NO_VIA_ADDRESS;
		}

		/* Send to the interface directly, no hassle */
		if (csp_send_direct(packet->id, packet, &route, 0) != CSP_ERR_NONE) {
			csp_log_warn("Router failed to send");
			csp_buffer_free(packet);
		}
	}

	return CSP_TASK_RETURN;

}

int csp_bridge_start(unsigned int task_stack_size, unsigned int task_priority, csp_iface_t * if_a, csp_iface_t * if_b) {

	/* Set static references to A/B side of bridge */
	bif_a.iface = if_a;
	bif_b.iface = if_b;

	static csp_thread_handle_t handle;
	int ret = csp_thread_create(csp_bridge, "BRIDGE", task_stack_size, NULL, task_priority, &handle);

	if (ret != 0) {
		csp_log_error("Failed to start task");
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;

}
