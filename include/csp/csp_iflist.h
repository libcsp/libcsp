#pragma once

#include <csp/csp_interface.h>

/**
   Add interface to the list.

   @param[in] iface interface. The interface must remain valid as long as the application is running.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_iflist_add(csp_iface_t * iface);

csp_iface_t * csp_iflist_get_by_name(const char *name);
csp_iface_t * csp_iflist_get_by_addr(uint16_t addr);
csp_iface_t * csp_iflist_get_by_subnet(uint16_t addr, csp_iface_t * from);
int csp_iflist_is_within_subnet(uint16_t addr, csp_iface_t * ifc);

csp_iface_t * csp_iflist_get(void);

/* Convert bytes to readable string */
unsigned long csp_bytesize(unsigned long bytes, char *postfix);

#if (CSP_ENABLE_CSP_PRINT)
void csp_iflist_print(void);
#else
inline void csp_iflist_print(void) {}
#endif
