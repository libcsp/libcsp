/****************************************************************************
 * **File:** csp/csp_iflist.h
 *
 * **Description:** Interfaces management
 ****************************************************************************/
#pragma once

#include <csp/csp_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add interface to the list.
 *
 * @warning Routing is enabled on the interface by default upon registration (i.e.
 * #csp_iface_t::is_routing_enabled is set to true unconditionally). This
 * preserves legacy behavior for interfaces that were registered before the
 * routing-enable flag existed. If you need routing to be initially disabled,
 * call csp_iflist_set_routing_enabled() explicitly after this function returns.
 *
 * @param[in] iface The interface must remain valid as long as the application is running.
 */
void csp_iflist_add(csp_iface_t * iface);

/**
 * Remove interface from the list.
 *
 * @param[in] ifc Interface to remove. NULL will be gracefully handled.
 */
void csp_iflist_remove(csp_iface_t * ifc);

csp_iface_t * csp_iflist_get_by_name(const char * name);
csp_iface_t * csp_iflist_get_by_addr(uint16_t addr);
csp_iface_t * csp_iflist_get_by_subnet(uint16_t addr, csp_iface_t * from);
csp_iface_t * csp_iflist_get_by_isdfl(csp_iface_t * ifc);
csp_iface_t * csp_iflist_get_by_index(int idx);
int csp_iflist_is_within_subnet(uint16_t addr, csp_iface_t * ifc);

csp_iface_t * csp_iflist_get(void);

/**
 * Convert bytes to readable string
 */
unsigned long csp_bytesize(unsigned long bytes, char *postfix);

/**
 * Enable or disable routing on an interface at runtime.
 *
 * When disabled, the interface is skipped by all routing paths (subnet lookup,
 * routing table, default interface fallback). The interface remains registered
 * and its statistics are preserved.
 *
 * @param[in] iface  Interface to modify. NULL is a no-op.
 * @param[in] enable true to enable routing, false to disable.
 */
void csp_iflist_set_routing_enabled(csp_iface_t * iface, bool enable);

/**
 * Runs over the list of interfaces, and if no default interface is found
 * set default on ALL interfaces
 */
void csp_iflist_check_dfl(void);

#if (CSP_ENABLE_CSP_PRINT)
void csp_iflist_print(void);
#else
inline void csp_iflist_print(void) {}
#endif

#ifdef __cplusplus
}
#endif
