#include <string.h> /* For memchr in csp_autoconfig.h */
#include <csp/csp_platform.h>
#include <csp/csp_interface.h>
#include <csp/csp_iflist.h>
#include <csp/csp_rtable.h>
#include <csp/arch/csp_malloc.h>
#include <csp/interfaces/csp_if_sdr.h>
#include <csp/drivers/sdr.h>
#include <sdr_driver.h>

int csp_uhf_open_and_add_interface(const sdr_uhf_conf_t *uhf_conf, const char *ifname, csp_iface_t **return_iface) {

    if (conf->uhf_baudrate == 0 || conf->uhf_baudrate >= SDR_UHF_END_BAUD) {
        return CSP_ERR_INVAL;
    }

    sdr_uhf_conf = csp_malloc(sizeof(sdr_uhf_conf_t));
    csp_iface_t *iface = csp_calloc(1, sizeof(csp_iface_t));
    sdr_interface_data_t *ifdata = csp_calloc(1, sizeof(sdr_interface_data_t));
    if (!sdr_uhf_conf || !iface || !ifdata) {
        csp_free(sdr_uhf_conf);
        csp_free(iface);
        csp_free(ifdata);
        sdr_uhf_conf = 0;
        return CSP_ERR_NOMEM;
    }

    memcpy(sdr_uhf_conf, uhf_conf, sizeof(sdr_uhf_conf_t));

    iface->name = ifname;
    iface->mtu = csp_buffer_data_size() + sizeof(csp_packet_t);

    iface->interface_data = sdr_uhf_conf;
    sdr_uhf_conf->if_data = ifdata;

    int rc = csp_sdr_driver_init(iface);
    if (rc) {
        csp_free(sdr_uhf_conf);
        sdr_uhf_conf = 0;
        csp_free(iface);
        csp_free(ifdata);
        return rc;
    }
    iface->nexthop = csp_sdr_tx;

    #ifndef CSP_POSIX
    if (strcmp(ifname, SDR_IF_LOOPBACK_NAME) == 0) {
        sdr_loopback_open(ifdata);
    }
    #endif // CSP_POSIX

    csp_iflist_add(iface);

    if (return_iface) {
        *return_iface = iface;
    }

    return CSP_ERR_NONE;
}
