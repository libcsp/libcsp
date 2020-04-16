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

#include <csp/csp_iflist.h>

#include <stdio.h>
#include <string.h>

#include <csp/csp_debug.h>

/* Interfaces are stored in a linked list */
static csp_iface_t * interfaces = NULL;

csp_iface_t * csp_iflist_get_by_name(const char *name) {
	csp_iface_t *ifc = interfaces;
	while(ifc) {
		if (strncasecmp(ifc->name, name, CSP_IFLIST_NAME_MAX) == 0) {
			break;
		}
		ifc = ifc->next;
	}
	return ifc;
}

int csp_iflist_add(csp_iface_t *ifc) {

	ifc->next = NULL;

	/* Add interface to pool */
	if (interfaces == NULL) {
		/* This is the first interface to be added */
		interfaces = ifc;
	} else {
		/* Insert interface last if not already in pool */
		csp_iface_t * last = NULL;
		for (csp_iface_t * i = interfaces; i != NULL; i = i->next) {
			if ((i == ifc) || (strncasecmp(ifc->name, i->name, CSP_IFLIST_NAME_MAX) == 0)) {
				return CSP_ERR_ALREADY;
			}
			last = i;
		}

		last->next = ifc;
	}

	return CSP_ERR_NONE;
}

csp_iface_t * csp_iflist_get(void)
{
    return interfaces;
}

#if (CSP_DEBUG)
int csp_bytesize(char *buffer, int buffer_len, unsigned long int bytes) {
	char postfix;
	double size;

	if (bytes >= 1048576) {
		size = bytes/1048576.0;
		postfix = 'M';
	} else if (bytes >= 1024) {
		size = bytes/1024.0;
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
		printf("%-10s tx: %05"PRIu32" rx: %05"PRIu32" txe: %05"PRIu32" rxe: %05"PRIu32"\r\n"
		       "           drop: %05"PRIu32" autherr: %05"PRIu32 " frame: %05"PRIu32"\r\n"
		       "           txb: %"PRIu32" (%s) rxb: %"PRIu32" (%s) MTU: %u\r\n\r\n",
		       i->name, i->tx, i->rx, i->tx_error, i->rx_error, i->drop,
		       i->autherr, i->frame, i->txbytes, txbuf, i->rxbytes, rxbuf, i->mtu);
		i = i->next;
	}
}
#endif

