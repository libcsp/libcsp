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
    bool is_zmq;
} bridge_interface_t;

static bridge_interface_t if_a;
static bridge_interface_t if_b;

static uint8_t get_mac(bridge_interface_t * iface, const csp_packet_t * packet) {

	if (iface->is_zmq) {
		return ((const csp_zmqhub_csp_packet_t *)packet)->mac;
	}
	return CSP_NODE_MAC;
}

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
#ifdef CSP_USE_PROMISC
		csp_promisc_add(packet);
#endif

		/* Find the opposing interface */
		csp_rtable_route_t route;
		if (input.interface == if_a.iface) {
			route.interface = if_b.iface;
			route.mac = get_mac(&if_a, packet);
		} else {
			route.interface = if_a.iface;
			route.mac = get_mac(&if_b, packet);
		}

		/* Send to the interface directly, no hassle */
		if (csp_send_direct(packet->id, packet, &route, 0) != CSP_ERR_NONE) {
			csp_log_warn("Router failed to send");
			csp_buffer_free(packet);
		}
	}

	return CSP_TASK_RETURN;

}

static bool is_zmq_interface(const char * ifname)
{
	if (ifname) {
		// if the interface contains zmq, we assume it is a ZMQ interface
		for (unsigned int i = 0; i < strlen(ifname); ++i) {
                    if (strncasecmp(&ifname[i], "zmq", 3) == 0 ) {
				return true;
			}
		}
	}
	return false;
}

int csp_bridge_start(unsigned int task_stack_size, unsigned int task_priority, csp_iface_t * _if_a, csp_iface_t * _if_b) {

	/* Set static references to A/B side of bridge */
	if_a.iface = _if_a;
	if_a.is_zmq = is_zmq_interface(_if_a->name);
	if_b.iface = _if_b;
	if_b.is_zmq = is_zmq_interface(_if_b->name);

	static csp_thread_handle_t handle;
	int ret = csp_thread_create(csp_bridge, "BRIDGE", task_stack_size, NULL, task_priority, &handle);

	if (ret != 0) {
		csp_log_error("Failed to start task");
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;

}
