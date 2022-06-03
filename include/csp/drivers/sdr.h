#ifndef CSP_DRIVERS_SDR_H
#define CSP_DRIVERS_SDR_H

#include <csp/csp.h>
#include "sdr_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

int csp_sdr_tx(const csp_route_t *ifroute, csp_packet_t *packet);

void csp_sdr_rx(void *cb_data, void *buf, size_t len, void *pxTaskWoken);

int csp_sdr_uart_driver_init(sdr_uhf_conf_t *sdr_conf);

#ifdef __cplusplus
}
#endif
#endif /* CSP_DRIVERS_SDR_H */
