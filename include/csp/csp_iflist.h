

#ifndef CSP_IFLIST_H_
#define CSP_IFLIST_H_

/**
   @file

   Interface list.

   Linked-list of interfaces in the system.

   This API is not thread-safe.
*/

#include <csp/csp_interface.h>



/**
   Add interface to the list.

   @param[in] iface interface. The interface must remain valid as long as the application is running.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_iflist_add(csp_iface_t * iface);

/**
   Get interface by name.

   @param[in] name interface name.
   @return Interface or NULL if not found.
*/
csp_iface_t * csp_iflist_get_by_name(const char *name);

/**
   Print list of interfaces to stdout.
*/
void csp_iflist_print(void);

/**
   Return list of interfaces.

   @return First interface or NULL, if no interfaces added.
*/
csp_iface_t * csp_iflist_get(void);

/**
   Convert bytes to readable string.
*/
int csp_bytesize(char *buffer, int buffer_len, unsigned long int bytes);
    

#endif
