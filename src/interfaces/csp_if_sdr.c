#include <string.h> /* For memchr in csp_autoconfig.h */
#include <csp/csp_platform.h>
#include <csp/csp_interface.h>
#include <csp/csp_iflist.h>
#include <csp/csp_rtable.h>
#include <csp/arch/csp_malloc.h>
#include <csp/drivers/usart.h>
#include <csp/interfaces/csp_if_sdr.h>
#include <csp/drivers/sdr.h>
#include "rfModeWrapper.h"
#include "error_correctionWrapper.h"
#include <csp/drivers/fec.h>
#include <csp/csp_buffer.h>

int csp_sdr_open_and_add_interface(const csp_sdr_conf_t *conf, const char *ifname, csp_iface_t **return_iface) {

    if (conf->baudrate == 0 || conf->baudrate >= SDR_UHF_END_BAUD) {
        return CSP_ERR_INVAL;
    }

    csp_sdr_conf_t *sdr_conf = csp_calloc(1, sizeof(csp_sdr_conf_t));
    memcpy(sdr_conf, conf, sizeof(csp_sdr_conf_t));

    csp_iface_t *iface = csp_calloc(1, sizeof(csp_iface_t));
    iface->name = ifname;
    iface->mtu = csp_buffer_data_size() + sizeof(csp_packet_t);
    iface->driver_data = (void *)sdr_conf;

    csp_sdr_interface_data_t *ifdata = csp_calloc(1, sizeof(csp_sdr_interface_data_t));
    iface->interface_data = (void *)ifdata;
    ifdata->uhf_baudrate = conf->baudrate;
    ifdata->mac_data = fec_create(RF_MODE_3, NO_FEC);
    ifdata->rx_mpdu = csp_calloc(conf->mtu, sizeof(int8_t));

    csp_sdr_driver_init(iface);

    iface->nexthop = csp_sdr_tx;

    csp_iflist_add(iface);

    if (return_iface) {
        *return_iface = iface;
    }

    return CSP_ERR_NONE;
}
