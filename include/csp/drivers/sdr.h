#ifndef CSP_DRIVERS_SDR_H
#define CSP_DRIVERS_SDR_H

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
    uint16_t mtu;
    sdr_uhf_baud_rate_t baudrate;
    int uart_baudrate;
    char *device_file;
    csp_usart_conf_t csp_usart_conf;
} csp_sdr_conf_t;

int csp_sdr_tx(const csp_route_t *ifroute, csp_packet_t *packet);

void csp_sdr_rx(void *cb_data, void *buf, size_t len, void *pxTaskWoken);

int csp_sdr_driver_init(csp_iface_t * iface);

#ifdef __cplusplus
}
#endif
#endif /* CSP_DRIVERS_SDR_H */
