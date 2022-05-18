#include <string.h>
#include <csp/csp_interface.h>
#include <csp/arch/csp_malloc.h>
#include <csp/drivers/sdr.h>
#include <csp/interfaces/csp_if_sdr.h>
#include <csp/arch/csp_queue.h>

static csp_iface_t *loop_iface;

static int sdr_loopback_tx(CSP_BASE_TYPE fd, const void *buf, size_t len) {
    csp_sdr_interface_data_t *ifdata = loop_iface->interface_data;

    uint8_t *data = (uint8_t *)buf;
    while (len--) {
        if (csp_queue_enqueue(ifdata->rx_queue, (const uint8_t *)data, QUEUE_NO_WAIT) != true) {
            return CSP_ERR_TIMEDOUT;
        }
        data++;
    }

    return CSP_ERR_NONE;
}

void sdr_loopback_open(csp_iface_t *iface) {
    csp_sdr_interface_data_t *ifdata = iface->interface_data;
    loop_iface = iface;
    iface->split_horizon_off = 0;
    ifdata->tx_func = sdr_loopback_tx;
}
