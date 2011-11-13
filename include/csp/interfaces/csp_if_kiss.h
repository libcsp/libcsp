
#ifndef _CSP_IF_KISS_H_
#define _CSP_IF_KISS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <csp/csp.h>
#include <csp/csp_interface.h>

extern csp_iface_t csp_if_kiss;

int csp_kiss_tx(csp_packet_t * packet, uint32_t timeout);

void csp_kiss_init(int handle);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_IF_KISS_H_
