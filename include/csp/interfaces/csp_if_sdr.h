#ifndef CSP_INTERFACES_CSP_IF_SDR_H
#define CSP_INTERFACES_CSP_IF_SDR_H

#include <csp/csp.h>
#include <sdr_driver.h>

int csp_sdr_open_and_add_interface(const sdr_conf_t *conf, const char *ifname, csp_iface_t **return_iface);

#endif /* CSP_INTERFACES_CSP_IF_SDR_H */
