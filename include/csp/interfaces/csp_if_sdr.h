#ifndef CSP_INTERFACES_CSP_IF_SDR_H
#define CSP_INTERFACES_CSP_IF_SDR_H

#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/csp_platform.h>
#include <csp/drivers/sdr.h>

#define CSP_IF_SDR_UHF_NAME "UHF"
#define CSP_IF_SDR_SBAND_NAME "S-BAND"
#define CSP_IF_SDR_LOOPBACK_NAME "LOOPBACK"

#define SDR_UHF_MAX_MTU 128
#define SDR_SBAND_MAX_MTU 128

#define QUEUE_NO_WAIT 0

/**
   Send MPDU frame (implemented by driver).

   @param[in] driver_data driver data from #csp_iface_t
   @param[in] data data to send
   @param[in] len length of \a data.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
typedef int (*csp_sdr_driver_tx_t)(CSP_BASE_TYPE fd, const void * data, size_t data_length);
typedef void (*csp_sdr_driver_rx_t) (void * data, uint8_t *buf, size_t len, void *pxTaskWoken);

typedef struct {
    sdr_uhf_baud_rate_t uhf_baudrate;
    int tx_timeout;
    CSP_BASE_TYPE fd;
    /** Low Level Transmit Function */
    csp_sdr_driver_tx_t tx_func;
    /** Low level Receive function */
    csp_sdr_driver_rx_t rx_func;
    csp_queue_handle_t rx_queue;
    void *mac_data;
} csp_sdr_interface_data_t;

int csp_sdr_open_and_add_interface(const csp_sdr_conf_t *conf, const char *ifname, csp_iface_t **return_iface);

void sdr_loopback_open(csp_iface_t*);

#endif /* CSP_INTERFACES_CSP_IF_SDR_H */
