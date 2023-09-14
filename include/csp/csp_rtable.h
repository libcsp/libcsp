/****************************************************************************
 * File: csp_rtable.h
 * Description: Routing table
 * The routing table maps a CSP destination address to an interface (and optional a via address).
 *
 * Normal routing: If the route's via address is set to #CSP_NO_VIA_ADDRESS, the packet will be sent directly to the destination address
 * specified in the CSP header, otherwise the packet will be sent the to the route's via address.
 ****************************************************************************/
#pragma once

#include <csp/csp_iflist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSP_NO_VIA_ADDRESS	0xFFFF

typedef struct csp_route_s {
	uint16_t address;
	uint16_t netmask;
   uint16_t via;
   csp_iface_t * iface;
} csp_route_t;

/**
 * Loop through routes backwards and find routes that match on addr and mask from start_route
 */
csp_route_t * csp_rtable_search_backward(csp_route_t * start_route);
csp_route_t * csp_rtable_find_route(uint16_t dest_address);

/**
 * Set route to destination address/node.
 *
 * Parameters:
 *	dest_address (uint16_t) [in]: destination address.
 *	mask (int) [in]: number of bits in netmask (set to -1 for maximum number of bits)
 *	ifc (csp_iface_t *) [in]: interface.
 *	via (uint16_t) [in]: assosicated via address.
 *
 * Returns:
 *	int: #CSP_ERR_NONE on success, or an error code.
 */
int csp_rtable_set(uint16_t dest_address, int netmask, csp_iface_t *ifc, uint16_t via);

#if (CSP_HAVE_STDIO)
/**
 * Save routing table as a string (readable format).
 * @see csp_rtable_load() for additional information, e.g. format.
 *
 * Parameters:
 *	buffer (char *) [out]: user supplied buffer.
 * 	buffer_size (size_t) [in]: size of \a buffer.
 *
 * Returns:
 *	int: #CSP_ERR_NONE on success, or an error code.
 */
int csp_rtable_save(char * buffer, size_t buffer_size);

/**
 * Load routing table from a string.
 * Table will be loaded on-top of existing routes, possibly overwriting existing entries.
 * Format: \<address\>[/mask] \<interface\> [via][, next entry]
 * Example: "0/0 CAN, 8 KISS, 10 I2C 10", same as "0/0 CAN, 8/5 KISS, 10/5 I2C 10".
 * @see csp_rtable_save(), csp_rtable_clear(), csp_rtable_free()
 *
 * Parameters:
 *	rtable (const char *) [in]: routing table (nul terminated)
 *
 * Returns:
 *	int: CSP_ERR or number of entries.
 */
int csp_rtable_load(const char * rtable);

/**
 * Check string for valid routing elements.
 *
 * Parameters:
 *	rtable (const char *) [in]: routing table (nul terminated)
 *
 * Returns:
 *	int: CSP_ERR or number of entries.
 */
int csp_rtable_check(const char * rtable);

#else
inline int csp_rtable_save(char * buffer, size_t buffer_size) { return CSP_ERR_NOSYS; }
inline int csp_rtable_load(const char * rtable) { return CSP_ERR_NOSYS; }
inline int csp_rtable_check(const char * rtable) { return CSP_ERR_NOSYS; }
#endif

/**
 * Clear routing table and add loopback route.
 * @see csp_rtable_free()
 */
void csp_rtable_clear(void);

/**
 * Clear/free all entries in the routing table.
 */
void csp_rtable_free(void);

/** Iterator for looping through the routing table. */
typedef bool (*csp_rtable_iterator_t)(void * ctx, csp_route_t * route);

/**
 * Iterate routing table.
 */
void csp_rtable_iterate(csp_rtable_iterator_t iter, void * ctx);

#if (CSP_ENABLE_CSP_PRINT)

/**
 * Print routing table
 */
void csp_rtable_print(void);

#else
inline void csp_rtable_print(void) {}
#endif

#ifdef __cplusplus
}
#endif
