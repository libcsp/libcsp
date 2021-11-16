#include <string.h> /* For memchr in csp_autoconfig.h */
#include <csp/csp_platform.h>
#include <csp/csp_interface.h>
#include <csp/csp_iflist.h>
#include <csp/csp_rtable.h>
#include <csp/arch/csp_malloc.h>
#include <csp/drivers/usart.h>
#include <csp/interfaces/csp_if_sdr.h>

int csp_sdr_tx(const csp_route_t *ifroute, csp_packet_t *packet) {
    csp_sdr_interface_data_t *ifdata = ifroute->iface->interface_data;
    void *packet_address = packet; 
    if (xQueueSendToBack(ifdata->tx_queue, &packet_address, ifdata->tx_timeout) != pdPASS) {
        return CSP_ERR_NOBUFS;
    }

    if (xSemaphoreTake(ifdata->tx_sema, portMAX_DELAY) != pdTRUE) {
        return CSP_ERR_TIMEDOUT;
    }
    return CSP_ERR_NONE;
}

static void csp_sdr_usart_rx(void *cb_data, uint8_t *buf, size_t len, void *pxTaskWoken) {
    csp_iface_t *iface = (csp_iface_t *) cb_data;
    csp_sdr_interface_data_t *ifdata = iface->interface_data;
    ifdata->rx_mpdu[ifdata->rx_mpdu_index] = *buf;
    ifdata->rx_mpdu_index++;
    if (ifdata->rx_mpdu_index >= SDR_UHF_MAX_MTU) {
        ifdata->rx_mpdu_index = 0;
        if (xQueueSend(ifdata->rx_queue, ifdata->rx_mpdu, QUEUE_NO_WAIT) != pdPASS) {
        return CSP_ERR_NOBUFS;
        }
    }
}

int csp_sdr_add_interface(csp_iface_t * iface) {
    if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
        return CSP_ERR_INVAL;
    }

    csp_sdr_interface_data_t *ifdata = iface->interface_data;
    if (strcmp(iface->name, CSP_IF_SDR_UHF_NAME) == 0) {
        ifdata->tx_mac = (csp_sdr_driver_tx_t) csp_usart_write;
        csp_usart_conf_t *conf = csp_calloc(1, sizeof(csp_usart_conf_t));
        if (!conf)
            return CSP_ERR_NOMEM;

        conf->device = "UART";
        conf->baudrate = ifdata->baudrate;
        conf->databits = 8;
        conf->stopbits = 2;
        int res = csp_usart_open(conf, csp_sdr_usart_rx, iface, NULL);
        if (res != CSP_ERR_NONE) {
            csp_free(conf);
            return res;
        }
    }
    else if (strcmp(iface->name, CSP_IF_SDR_LOOPBACK_NAME) == 0) {
        ifdata->mac_data = sdr_loopback_open(iface);
    }

    iface->nexthop = csp_sdr_tx;

    return csp_iflist_add(iface);
}
