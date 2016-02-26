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

#include <stdio.h>
#include <string.h>
#include <csp/csp.h>
#include <alloca.h>
#include <csp/arch/csp_malloc.h>
#include <csp/interfaces/csp_if_lo.h>

/* Local typedef for routing table */
typedef struct __attribute__((__packed__)) csp_rtable_s {
	uint8_t address;
	uint8_t netmask;
	uint8_t mac;
	csp_iface_t * interface;
	struct csp_rtable_s * next;
} csp_rtable_t;

/* Routing entries are stored in a linked list*/
static csp_rtable_t * rtable = NULL;

static csp_rtable_t * csp_rtable_find(uint8_t addr, uint8_t netmask, uint8_t exact) {

	/* Remember best result */
	csp_rtable_t * best_result = NULL;
	uint8_t best_result_mask = 0;

	/* Start search */
	csp_rtable_t * i = rtable;
	while(i) {

		/* Look for exact match */
		if (i->address == addr && i->netmask == netmask) {
			best_result = i;
			break;
		}

		/* Try a CIDR netmask match */
		if (!exact) {
			uint8_t hostbits = (1 << (CSP_ID_HOST_SIZE - i->netmask)) - 1;
			uint8_t netbits = ~hostbits;
			//printf("Netbits %x Hostbits %x\r\n", netbits, hostbits);

			/* Match network addresses */
			uint8_t net_a = i->address & netbits;
			uint8_t net_b = addr & netbits;
			//printf("A: %hhx, B: %hhx\r\n", net_a, net_b);

			/* We have a match */
			if (net_a == net_b) {
				if (i->netmask >= best_result_mask) {
					//printf("Match best result %u %u\r\n", best_result_mask, i->netmask);
					best_result = i;
					best_result_mask = i->netmask;
				}
			}

		}

		i = i->next;

	}

#if 0
	if (best_result)
		csp_debug(CSP_PACKET, "Using routing entry: %u/%u dev %s m:%u\r\n", best_result->address, best_result->netmask, best_result->interface->name, best_result->mac);
#endif

	return best_result;

}

void csp_rtable_clear(void) {
	for (csp_rtable_t * i = rtable; (i);) {
		void * freeme = i;
		i = i->next;
		csp_free(freeme);
	}
	rtable = NULL;

	/* Set loopback up again */
	csp_rtable_set(csp_get_address(), CSP_ID_HOST_SIZE, &csp_if_lo, CSP_NODE_MAC);

}

static int csp_rtable_parse(char * buffer, int dry_run) {

	int valid_entries = 0;

	/* Copy string before running strtok */
	char * str = alloca(strlen(buffer) + 1);
	memcpy(str, buffer, strlen(buffer) + 1);

	/* Get first token */
	str = strtok(str, ",");

	while ((str) && (strlen(str) > 1)) {
		int address = 0, netmask = 0, mac = 255;
		char name[100] = {};
		if (sscanf(str, "%u/%u %s %u", &address, &netmask, name, &mac) != 4) {
			if (sscanf(str, "%u/%u %s", &address, &netmask, name) != 3) {
				csp_log_error("Parse error %s", str);
				return -1;
			}
		}
		//printf("Parsed %u/%u %u %s\r\n", address, netmask, mac, name);
		csp_iface_t * ifc = csp_iflist_get_by_name(name);
		if (ifc) {
			if (dry_run == 0)
				csp_rtable_set(address, netmask, ifc, mac);
		} else {
			csp_log_error("Unknown interface %s", name);
			return -1;
		}
		valid_entries++;
		str = strtok(NULL, ",");
	}

	return valid_entries;
}

void csp_rtable_load(char * buffer) {
	csp_rtable_parse(buffer, 0);
}

int csp_rtable_check(char * buffer) {
	return csp_rtable_parse(buffer, 1);
}

int csp_rtable_save(char * buffer, int maxlen) {
	int len = 0;
	for (csp_rtable_t * i = rtable; (i); i = i->next) {
		if (i->mac != CSP_NODE_MAC) {
			len += snprintf(buffer + len, maxlen - len, "%u/%u %s %u, ", i->address, i->netmask, i->interface->name, i->mac);
		} else {
			len += snprintf(buffer + len, maxlen - len, "%u/%u %s, ", i->address, i->netmask, i->interface->name);
		}
	}
	return len;
}

csp_iface_t * csp_rtable_find_iface(uint8_t id) {
	csp_rtable_t * entry = csp_rtable_find(id, CSP_ID_HOST_SIZE, 0);
	if (entry == NULL)
		return NULL;
	return entry->interface;
}

uint8_t csp_rtable_find_mac(uint8_t id) {
	csp_rtable_t * entry = csp_rtable_find(id, CSP_ID_HOST_SIZE, 0);
	if (entry == NULL)
		return 255;
	return entry->mac;
}

int csp_rtable_set(uint8_t _address, uint8_t _netmask, csp_iface_t *ifc, uint8_t mac) {

	if (ifc == NULL)
		return CSP_ERR_INVAL;

	/* Set default route in the old way */
	int address, netmask;
	if (_address == CSP_DEFAULT_ROUTE) {
		netmask = 0;
		address = 0;
	} else {
		netmask = _netmask;
		address = _address;
	}

	/* Fist see if the entry exists */
	csp_rtable_t * entry = csp_rtable_find(address, netmask, 1);

	/* If not, create a new one */
	if (!entry) {
		entry = csp_malloc(sizeof(csp_rtable_t));
		if (entry == NULL)
			return CSP_ERR_NOMEM;

		entry->next = NULL;
		/* Add entry to linked-list */
		if (rtable == NULL) {
			/* This is the first interface to be added */
			rtable = entry;
		} else {
			/* One or more interfaces were already added */
			csp_rtable_t * i = rtable;
			while (i->next)
				i = i->next;
			i->next = entry;
		}
	}

	/* Fill in the data */
	entry->address = address;
	entry->netmask = netmask;
	entry->interface = ifc;
	entry->mac = mac;

	return CSP_ERR_NONE;
}

void csp_rtable_print(void) {

	for (csp_rtable_t * i = rtable; (i); i = i->next) {
		if (i->mac == 255) {
			printf("%u/%u %s\r\n", i->address, i->netmask, i->interface->name);
		} else {
			printf("%u/%u %s %u\r\n", i->address, i->netmask, i->interface->name, i->mac);
		}
	}

}

