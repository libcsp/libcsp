

#include <csp/interfaces/csp_if_lo.h>
#include <csp/arch/csp_time.h>
#include <csp/csp_id.h>
#include <csp_autoconfig.h>
#include "csp_conn.h"
#include "csp_qfifo.h"
#include "csp_port.h"
#include "csp_rdp_queue.h"

csp_conf_t csp_conf = {
	.version = 2,
	.address = 0,
	.hostname = "",
	.model = "",
	.revision = "",
	.conn_dfl_so = CSP_O_NONE,
	.dedup = CSP_DEDUP_OFF};

uint16_t csp_get_address(void) {
	return csp_conf.address;
}

void csp_init(void) {

	csp_buffer_init();
	csp_conn_init();
	csp_qfifo_init();
#if (CSP_USE_RDP)
	csp_rdp_queue_init();
#endif

	/* Loopback */
	csp_if_lo.netmask = csp_id_get_host_bits();
	csp_iflist_add(&csp_if_lo);

}

void csp_free_resources(void) {

	csp_rtable_free();
}

const csp_conf_t * csp_get_conf(void) {
	return &csp_conf;
}
