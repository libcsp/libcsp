#ifndef CSP_DRIVERS_SDR_H
#define CSP_DRIVERS_SDR_H

#include <csp/csp_types.h>
#include <csp/csp_interface.h>
#include "sdr_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

int csp_sdr_tx(const csp_route_t *ifroute, csp_packet_t *packet);

void csp_sdr_rx(void *cb_data, void *buf, size_t len, void *pxTaskWoken);

int csp_sdr_driver_init(csp_iface_t *iface);

#ifdef __cplusplus
extern "C" {
#endif

#endif /* CSP_DRIVERS_SDR_H */
