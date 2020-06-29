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

/**
   @file

   Routing table.

   The routing table maps a CSP destination address to an interface (and optional a via address).

   Normal routing: If the route's via address is set to #CSP_NO_VIA_ADDRESS, the packet will be sent directly to the destination address
   specified in the CSP header, otherwise the packet will be sent the to the route's via address.
*/

#include <csp/csp_iflist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   No via address specified for the route, use CSP header destination.
*/
#define CSP_NO_VIA_ADDRESS	0xFF

/**
   Legacy definition for #CSP_NO_VIA_ADDRESS.
*/
#define CSP_NODE_MAC	CSP_NO_VIA_ADDRESS
    
/**
   Route entry.
   @see csp_rtable_find_route().
*/
struct csp_route_s {
    /** Which interface to route packet on */
    csp_iface_t * iface;
    /** If different from #CSP_NO_VIA_ADDRESS, send packet to this address, instead of the destination address in the CSP header. */
    uint8_t via;
};

/**
   Find route to address/node.
   @param[in] dest_address destination address.
   @return Route or NULL if no route found.
*/
const csp_route_t * csp_rtable_find_route(uint8_t dest_address);

/**
   Set route to destination address/node.
   @param[in] dest_address destination address.
   @param[in] mask number of bits in netmask
   @param[in] ifc interface.
   @param[in] via assosicated via address.
   @return #CSP_ERR_NONE on success, or an error code.
*/
int csp_rtable_set(uint8_t dest_address, uint8_t mask, csp_iface_t *ifc, uint8_t via);

/**
   Save routing table as a string (readable format).
   @see csp_rtable_load() for additional information, e.g. format.
   @param[out] buffer user supplied buffer.
   @param[in] buffer_size size of \a buffer.
   @return #CSP_ERR_NONE on success, or an error code.
*/
int csp_rtable_save(char * buffer, size_t buffer_size);

/**
   Load routing table from a string.
   Table will be loaded on-top of existing routes, possibly overwriting existing entries.
   Format: \<address\>[/mask] \<interface\> [via][, next entry]
   Example: "0/0 CAN, 8 KISS, 10 I2C 10", same as "0/0 CAN, 8/5 KISS, 10/5 I2C 10".
   @see csp_rtable_save(), csp_rtable_clear(), csp_rtable_free()
   @param[in] rtable routing table (nul terminated)
   @return @ref CSP_ERR or number of entries.
*/
int csp_rtable_load(const char * rtable);

/**
   Load routing table from a string.
   Table will be loaded on-top of existing routes, possibly overwriting existing entries.
   Format: \<address\>[/mask] \<interface\> [via][, next entry]
   Example: "0/0 CAN, 8 KISS, 10 I2C 10", same as "0/0 CAN, 8/5 KISS, 10/5 I2C 10".
   @see csp_rtable_save(), csp_rtable_clear(), csp_rtable_free()
   @param[in] rtable routing table (nul terminated)
   @return @ref CSP_ERR or number of entries.
*/
int csp_rtable_load(const char * rtable);

/**
   Check string for valid routing elements.
   @param[in] rtable routing table (nul terminated)
   @return @ref CSP_ERR or number of entries.
*/
int csp_rtable_check(const char * rtable);

/**
   Clear routing table and add loopback route.
   @see csp_rtable_free()
*/
void csp_rtable_clear(void);

/**
   Clear/free all entries in the routing table.
*/
void csp_rtable_free(void);

/**
   Print routing table
*/
void csp_rtable_print(void);

/** Iterator for looping through the routing table. */
typedef bool (*csp_rtable_iterator_t)(void * ctx, uint8_t address, uint8_t mask, const csp_route_t * route);

/**
   Iterate routing table.
*/
void csp_rtable_iterate(csp_rtable_iterator_t iter, void * ctx);

/**
   Set route to destination address/node.
   @deprecated Use csp_rtable_set() instead.
   @param[in] dest_address destination address.
   @param[in] ifc interface.
   @param[in] via assosicated via address.
   @return #CSP_ERR_NONE on success, or an error code.
*/
static inline int csp_route_set(uint8_t dest_address, csp_iface_t *ifc, uint8_t via) {
    return csp_rtable_set(dest_address, CSP_ID_HOST_SIZE, ifc, via);
}

/**
   Print routing table.
   @deprecated Use csp_rtable_print() instead.
*/
static inline void csp_route_print_table() {
    csp_rtable_print();
}

/**
   Print list of interfaces.
   @deprecated Use csp_iflist_print() instead.
*/
static inline void csp_route_print_interfaces(void) {
    csp_iflist_print();
}

#ifdef __cplusplus
}
#endif
#endif
