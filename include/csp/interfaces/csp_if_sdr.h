#ifndef CSP_INTERFACES_CSP_IF_SDR_H
#define CSP_INTERFACES_CSP_IF_SDR_H

#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <string.h> // for memchr in csp_autoconfig.h
#include <csp/csp_platform.h>

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
typedef int (*csp_sdr_driver_tx_t)(int, const void * data, size_t len);

enum {
    SDR_CONF_FEC = 1 << 0,
};

typedef struct {
    /* Configuration */
    uint16_t config_flags;
    uint16_t mtu;
    uint32_t baudrate;
    /** Transmitter fields */
    csp_queue_handle_t tx_queue;
    int tx_timeout;
    /** Semaphore to block CSP sender until data has been queued for sending */
    csp_bin_sem_handle_t tx_sema;
    /** Low level transmit function */
    csp_sdr_driver_tx_t tx_mac;
    void *mac_data;
    /** Receiver fields */
    csp_queue_handle_t rx_queue;
    /** Low level buffer state */
    uint16_t rx_mpdu_index;
    uint8_t rx_mpdu[SDR_UHF_MAX_MTU];
} csp_sdr_interface_data_t;

int csp_sdr_add_interface(csp_iface_t *iface);

void* sdr_loopback_open(csp_iface_t *iface);

#endif /* CSP_INTERFACES_CSP_IF_SDR_H */
