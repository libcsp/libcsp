#include <FreeRTOS.h>
#include <os_task.h>
#include <os_queue.h>
#include <string.h>
#include <csp/csp_interface.h>
#include <csp/arch/csp_malloc.h>
#include <csp/drivers/sdr.h>
#include <util/service_utilities.h>

#define QUEUE_NO_WAIT 0

static int sdr_loopback_tx(void *iface, const uint8_t *data, size_t len) {
    csp_sdr_interface_data_t *ifdata = ((csp_iface_t *) iface)->interface_data;

    /* The UHF payload is always MTU bytes */
    if (xQueueSend(ifdata->rx_queue, data, QUEUE_NO_WAIT) != pdPASS) {
        return CSP_ERR_NOBUFS;
    }

    return CSP_ERR_NONE;
}

void* sdr_loopback_open(csp_iface_t *iface) {
    csp_sdr_interface_data_t *ifdata = iface->interface_data;

    iface->split_horizon_off = 0;
    ifdata->tx_mac = sdr_loopback_tx;
    return iface;
}
