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

#ifndef CSP_RTABLE_H_
#define CSP_RTABLE_H_

#include <csp/csp_types.h>

#define CSP_NODE_MAC				0xFF
#define CSP_ROUTE_COUNT				(CSP_ID_HOST_MAX + 2)
#define CSP_ROUTE_TABLE_SIZE		5 * CSP_ROUTE_COUNT

/**
 * Find outgoing interface in routing table
 * @param id Destination node
 * @return pointer to outgoing interface or NULL
 */
csp_iface_t * csp_rtable_find_iface(uint8_t id);

/**
 * Find MAC address associated with node
 * @param id Destination node
 * @return MAC address
 */
uint8_t csp_rtable_find_mac(uint8_t id);

/**
 * Setup routing entry
 * @param node Host
 * @param mask Number of bits in netmask
 * @param ifc Interface
 * @param mac MAC address
 * @return CSP error type
 */
int csp_rtable_set(uint8_t node, uint8_t mask, csp_iface_t *ifc, uint8_t mac);

/**
 * Print routing table to stdout
 */
void csp_rtable_print(void);


/**
 * Load the routing table from a buffer
 * (deprecated, please use new csp_rtable_load)
 *
 * Warning:
 * The table will be RAW from memory and contains direct pointers, not interface names.
 * Therefore it's very important that a saved routing table is deleted after a firmware update
 *
 * @param route_table_in pointer to routing table buffer
 */
void csp_route_table_load(uint8_t route_table_in[CSP_ROUTE_TABLE_SIZE]);

/**
 * Save the routing table to a buffer
 * (deprecated, please use new csp_rtable_save)
 *
 * Warning:
 * The table will be RAW from memory and contains direct pointers, not interface names.
 * Therefore it's very important that a saved routing table is deleted after a firmware update
 *
 * @param route_table_out pointer to routing table buffer
 */
void csp_route_table_save(uint8_t route_table_out[CSP_ROUTE_TABLE_SIZE]);

/**
 * Save routing table as a string to a buffer, which can be parsed
 * again by csp_rtable_load.
 * @param buffer pointer to buffer
 * @param maxlen length of buffer
 * @return length of saved string
 */
int csp_rtable_save(char * buffer, int maxlen);

/**
 * Load routing table from a string in the format
 * %u/%u %s %u
 * - Address
 * - Netmask
 * - Ifname
 * - Mac Address (this field is optional)
 * An example routing string is "0/0 I2C, 8/2 KISS"
 * The string must be \0 null terminated
 * The string must NOT be const.
 * @param buffer Pointer to string
 */
void csp_rtable_load(char * buffer);

/**
 * Check string for valid routing table
 * @param buffer Pointer to string
 * @return number of valid entries found
 */
int csp_rtable_check(char * buffer);

/**
 * Clear routing table:
 * This could be done before load to ensure an entire clean table is loaded.
 */
void csp_rtable_clear(void);

/**
 * Setup routing entry to single node
 * (deprecated, please use csp_rtable_set)
 *
 * @param node Host
 * @param ifc Interface
 * @param mac MAC address
 * @return CSP error type
 */
#define csp_route_set(node, ifc, mac) csp_rtable_set(node, CSP_ID_HOST_SIZE, ifc, mac)

/**
 * Print routing table
 * (deprecated, please use csp_rtable_print)
 */
#define csp_route_print_table() csp_rtable_print();

/**
 * Print list of interfaces
 * (deprecated, please use csp_iflist_print)
 */
#define csp_route_print_interfaces() csp_iflist_print();

#endif /* CSP_RTABLE_H_ */
