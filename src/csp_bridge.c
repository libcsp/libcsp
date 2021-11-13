#include "csp_qfifo.h"
#include "csp_io.h"
#include "csp_promisc.h"

typedef struct {
	csp_iface_t * iface;
} bridge_interface_t;

static bridge_interface_t bif_a;
static bridge_interface_t bif_b;

void csp_bridge_set_interfaces(csp_iface_t * if_a, csp_iface_t * if_b) {

	bif_a.iface = if_a;
	bif_b.iface = if_b;
}

void csp_bridge_work(void) {

	/* Get next packet to route */
	csp_qfifo_t input;
	if (csp_qfifo_read(&input) != CSP_ERR_NONE) {
		return;
	}

	csp_packet_t * packet = input.packet;

	csp_log_packet("INP: S %u, D %u, Dp %u, Sp %u, Pr %u, Fl 0x%02X, Sz %" PRIu16,
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
	if (csp_send_direct(packet->id, packet, &route) != CSP_ERR_NONE) {
		csp_log_warn("Router failed to send");
		csp_buffer_free(packet);
	}
}
