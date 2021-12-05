

#include <csp/csp_iflist.h>
#include <csp/csp_id.h>

#include <string.h>

#include <csp_autoconfig.h>
#include <csp/csp_debug.h>

/* Interfaces are stored in a linked list */
static csp_iface_t * interfaces = NULL;

csp_iface_t * csp_iflist_get_by_subnet(uint16_t addr) {

	csp_iface_t * ifc = interfaces;
	while (ifc) {

		/* Reject searches involving subnets, if the netmask is invalud */
		if (ifc->netmask == 0) {
			ifc = ifc->next;
			continue;
		}

		/* Look if address is within subnet */
		uint16_t netmask = ((1 << ifc->netmask) - 1) << (csp_id_get_host_bits() - ifc->netmask);
		uint16_t network_a = ifc->addr & netmask;
		uint16_t network_b = addr & netmask;
		if (network_a == network_b) {
			return ifc;
		}

		ifc = ifc->next;
	}
	
	return NULL;

}

csp_iface_t * csp_iflist_get_by_addr(uint16_t addr) {

	csp_iface_t * ifc = interfaces;
	while (ifc) {
		if (ifc->addr == addr) {
			return ifc;
		}
		ifc = ifc->next;
	}

	return NULL;

}

csp_iface_t * csp_iflist_get_by_name(const char * name) {
	csp_iface_t * ifc = interfaces;
	while (ifc) {
		if (strncmp(ifc->name, name, CSP_IFLIST_NAME_MAX) == 0) {
			return ifc;
		}
		ifc = ifc->next;
	}
	return NULL;
}

int csp_iflist_add(csp_iface_t * ifc) {

	ifc->next = NULL;

	/* Add interface to pool */
	if (interfaces == NULL) {
		/* This is the first interface to be added */
		interfaces = ifc;
	} else {
		/* Insert interface last if not already in pool */
		csp_iface_t * last = NULL;
		for (csp_iface_t * i = interfaces; i != NULL; i = i->next) {
			if ((i == ifc) || (strncmp(ifc->name, i->name, CSP_IFLIST_NAME_MAX) == 0)) {
				return CSP_ERR_ALREADY;
			}
			last = i;
		}

		last->next = ifc;
	}

	return CSP_ERR_NONE;
}

csp_iface_t * csp_iflist_get(void) {
	return interfaces;
}

#if (CSP_HAVE_STDIO)
int csp_bytesize(char * buffer, int buffer_len, unsigned long int bytes) {
	char postfix;
	double size;

	if (bytes >= 1048576) {
		size = bytes / 1048576.0;
		postfix = 'M';
	} else if (bytes >= 1024) {
		size = bytes / 1024.0;
		postfix = 'K';
	} else {
		size = bytes;
		postfix = 'B';
	}

	return snprintf(buffer, buffer_len, "%.1f%c", size, postfix);
}

void csp_iflist_print(void) {
	csp_iface_t * i = interfaces;
	char txbuf[25], rxbuf[25];

	while (i) {
		csp_bytesize(txbuf, sizeof(txbuf), i->txbytes);
		csp_bytesize(rxbuf, sizeof(rxbuf), i->rxbytes);
		csp_print("%-10s addr: %"PRIu16" netmask: %"PRIu16" mtu: %"PRIu16"\r\n"
			   "           tx: %05" PRIu32 " rx: %05" PRIu32 " txe: %05" PRIu32 " rxe: %05" PRIu32 "\r\n"
			   "           drop: %05" PRIu32 " autherr: %05" PRIu32 " frame: %05" PRIu32 "\r\n"
			   "           txb: %" PRIu32 " (%s) rxb: %" PRIu32 " (%s) \r\n\r\n",
			   i->name, i->addr, i->netmask, i->mtu, i->tx, i->rx, i->tx_error, i->rx_error, i->drop,
			   i->autherr, i->frame, i->txbytes, txbuf, i->rxbytes, rxbuf);
		i = i->next;
	}
}
#else
void csp_iflist_print(void) {};
#endif
