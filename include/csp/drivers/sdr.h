#ifndef CSP_DRIVERS_SDR_H
#define CSP_DRIVERS_SDR_H

#include <csp/interfaces/csp_if_sdr.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SDR_UHF_1200_BAUD = 0,
    SDR_UHF_2400_BAUD,
    SDR_UHF_4800_BAUD,
    SDR_UHF_9600_BAUD,
    SDR_UHF_19200_BAUD,
    SDR_UHF_END_BAUD,
} sdr_uhf_baud_rate_t;

typedef struct csp_sdr_conf {
    uint16_t use_fec;
    uint16_t mtu;
    sdr_uhf_baud_rate_t baudrate;
} csp_sdr_conf_t;

int csp_sdr_open_and_add_interface(const csp_sdr_conf_t *conf, const char *ifname, csp_iface_t **return_iface);

#ifdef __cplusplus
}
#endif
#endif /* CSP_DRIVERS_SDR_H */
