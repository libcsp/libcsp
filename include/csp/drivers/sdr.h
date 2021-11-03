#ifndef CSP_DRIVERS_SDR_H
#define CSP_DRIVERS_SDR_H

#include <csp/interfaces/csp_if_sdr.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct csp_sdr_conf {
    uint16_t use_fec;
    uint16_t mtu;
    uint32_t baudrate;
} csp_sdr_conf_t;

int csp_sdr_open_and_add_interface(const csp_sdr_conf_t *conf, const char *ifname, csp_iface_t **return_iface);

#ifdef __cplusplus
}
#endif
#endif /* CSP_DRIVERS_SDR_H */
