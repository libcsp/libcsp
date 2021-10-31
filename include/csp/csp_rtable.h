

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



/**
   No via address specified for the route, use CSP header destination.
*/
#define CSP_NO_VIA_ADDRESS	0xFFFF

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
    uint16_t via;
};

/**
   Find route to address/node.
   @param[in] dest_address destination address.
   @return Route or NULL if no route found.
*/
const csp_route_t * csp_rtable_find_route(uint16_t dest_address);

/**
   Set route to destination address/node.
   @param[in] dest_address destination address.
   @param[in] mask number of bits in netmask (set to -1 for maximum number of bits)
   @param[in] ifc interface.
   @param[in] via assosicated via address.
   @return #CSP_ERR_NONE on success, or an error code.
*/
int csp_rtable_set(uint16_t dest_address, int netmask, csp_iface_t *ifc, uint16_t via);

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
typedef bool (*csp_rtable_iterator_t)(void * ctx, uint16_t address, uint16_t mask, const csp_route_t * route);

/**
   Iterate routing table.
*/
void csp_rtable_iterate(csp_rtable_iterator_t iter, void * ctx);

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


#endif
