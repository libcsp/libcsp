#include "csp_cmp_internal.h"

#include <endian.h>

int csp_cmp_if_stats_handler(csp_packet_t * packet) {

	struct csp_cmp_if_stats_msg * cmp = (struct csp_cmp_if_stats_msg *)packet->data;

	if (csp_cmp_check_len(packet, offsetof(struct csp_cmp_if_stats_msg, tx)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	csp_iface_t * ifc = csp_iflist_get_by_name(cmp->interface);
	if (ifc == NULL) {
		return CSP_ERR_INVAL;
	}

	cmp->tx = htobe32(ifc->tx);
	cmp->rx = htobe32(ifc->rx);
	cmp->tx_error = htobe32(ifc->tx_error);
	cmp->rx_error = htobe32(ifc->rx_error);
	cmp->drop = htobe32(ifc->drop);
	cmp->autherr = htobe32(ifc->autherr);
	cmp->frame = htobe32(ifc->frame);
	cmp->txbytes = htobe32(ifc->txbytes);
	cmp->rxbytes = htobe32(ifc->rxbytes);
	cmp->irq = htobe32(ifc->irq);

	packet->length = sizeof(*cmp);

	return CSP_ERR_NONE;
}
