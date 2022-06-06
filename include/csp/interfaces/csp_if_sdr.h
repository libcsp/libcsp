#ifndef CSP_INTERFACES_CSP_IF_SDR_H
#define CSP_INTERFACES_CSP_IF_SDR_H

#include <csp/csp.h>
#include <sdr_driver.h>

int csp_uhf_open_and_add_interface(const sdr_uhf_conf_t *conf, const char *ifname, csp_iface_t **return_iface);
int csp_if_sdr_tx(const csp_route_t *ifroute, csp_packet_t *packet);
void csp_if_sdr_rx(void *udata, uint8_t *data, size_t len);

#endif /* CSP_INTERFACES_CSP_IF_SDR_H */
