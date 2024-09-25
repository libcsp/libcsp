#include "csp_macro.h"

#include "csp_qfifo.h"
#include "csp_io.h"
#include "csp_promisc.h"
#include "csp_dedup.h"
#include <csp/arch/csp_time.h>

static csp_iface_t * bif_a;
static csp_iface_t * bif_b;

void csp_bridge_set_interfaces(csp_iface_t * if_a, csp_iface_t * if_b) {

	bif_a = if_a;
	bif_b = if_b;
}

__weak void csp_input_hook(csp_iface_t * iface, csp_packet_t * packet) {
	csp_print_packet("INP: S %u, D %u, Dp %u, Sp %u, Pr %u, Fl 0x%02X, Sz %" PRIu16 " VIA: %s, Tms %u\n",
				   packet->id.src, packet->id.dst, packet->id.dport,
				   packet->id.sport, packet->id.pri, packet->id.flags, packet->length, iface->name, csp_get_ms());
}

void csp_bridge_work(void) {

	if ((bif_a == NULL) || (bif_b == NULL)) {
		csp_print("Bridge interfaces are not setup yet. "
				  "Make sure to call csp_bridge_set_interfaces()\n");
		return;
	}

	/* Get next packet to bridge */
	csp_qfifo_t input;
	if (csp_qfifo_read(&input) != CSP_ERR_NONE) {
		return;
	}

	csp_packet_t * packet = input.packet;
	if (packet == NULL) {
		csp_print("Packet of router queue item is NULL\n");
		return;
	}

	if (csp_dedup_is_duplicate(packet)) {
		csp_print("Retrieved packet is a duplicate\n");
		csp_buffer_free(packet);
		return;
	}

	csp_input_hook(input.iface, packet);

	/* Here there be promiscuous mode */
#if (CSP_USE_PROMISC)
	csp_promisc_add(packet);
#endif

	/* Find the opposing interface */
	csp_iface_t * destif;
	if (input.iface == bif_a) {
		destif = bif_b;
	} else {
		destif = bif_a;
	}

	/* Send to the interface directly, no hassle */
	csp_send_direct_iface(&packet->id, packet, destif, CSP_NO_VIA_ADDRESS, 0);

}
