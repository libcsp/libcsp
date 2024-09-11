

#include <csp/interfaces/csp_if_lo.h>
#include <csp/csp.h>

/**
 * Loopback interface transmit function
 * @param packet Packet to transmit
 * @return 1 if packet was successfully transmitted, 0 on error
 */
static int csp_lo_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	/* Send back into CSP, notice calling from task so last argument must be NULL! */
	csp_qfifo_write(packet, &csp_if_lo, NULL);

	return CSP_ERR_NONE;
}

/* Interface definition */
csp_iface_t csp_if_lo = {
	.name = CSP_IF_LOOPBACK_NAME,
	.nexthop = csp_lo_tx,
	.addr = 0,
	.is_default = 0
};
