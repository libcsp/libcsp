#include <string.h> /* For memchr in csp_autoconfig.h */
#include <csp/csp_platform.h>
#include <csp/csp_interface.h>
#include <csp/csp_iflist.h>
#include <csp/csp_rtable.h>
#include <csp/arch/csp_malloc.h>
#include <csp/interfaces/csp_if_sdr.h>
#include <csp/drivers/sdr.h>
#include <csp/csp_endian.h>
#include <sdr_driver.h>

int csp_if_sdr_tx(const csp_route_t *ifroute, csp_packet_t *packet) {
    sdr_interface_data_t *ifdata = (sdr_interface_data_t *)ifroute->iface->interface_data;
    uint16_t len = packet->length;
	packet->id.ext = csp_hton32(packet->id.ext);
	packet->length = csp_hton16(len);

    if (sdr_uhf_tx(ifdata, (uint8_t *)packet, len) == 0) {
        return CSP_ERR_NONE;
    } else {
        return CSP_ERR_TX;
    }
}

void csp_if_sdr_rx(void *udata, uint8_t *data, size_t len) {
    sdr_interface_data_t *ifdata = (sdr_interface_data_t *)udata;
    csp_iface_t *iface = ifdata->iface;
    csp_packet_t *packet = (csp_packet_t *)data;
    packet->length = csp_ntoh16(packet->length);
    packet->id.ext = csp_ntoh32(packet->id.ext);
    csp_packet_t *clone = csp_buffer_clone(packet);
    csp_qfifo_write(clone, iface, NULL);
}

int csp_uhf_open_and_add_interface(const sdr_uhf_conf_t *conf, const char *ifname, csp_iface_t **return_iface) {
    if (conf->uhf_baudrate == 0 || conf->uhf_baudrate >= SDR_UHF_END_BAUD) {
        return CSP_ERR_INVAL;
    }

    csp_iface_t *iface = csp_calloc(1, sizeof(csp_iface_t));
    sdr_interface_data_t *ifdata = sdr_uhf_interface_init(conf, ifname);
    if (!iface || !ifdata) {
        csp_free(ifdata->sdr_conf);
        csp_free(iface);
        csp_free(ifdata);
        return CSP_ERR_NOMEM;
    }
    iface->interface_data = ifdata;

    iface->name = ifname;
    iface->mtu = csp_buffer_data_size() + sizeof(csp_packet_t);

    sdr_uhf_conf_t *sdr_conf = ifdata->sdr_conf;
    sdr_conf->rx_callback = csp_if_sdr_rx;

    iface->nexthop = csp_if_sdr_tx;
    csp_iflist_add(iface);

    ifdata->iface = (void *)iface;

    if (return_iface) {
        *return_iface = iface;
    }

    return CSP_ERR_NONE;
}
