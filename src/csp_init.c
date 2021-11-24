

#include <csp/interfaces/csp_if_lo.h>
#include <csp/arch/csp_time.h>
#include <csp/csp_id.h>
#include "csp_conn.h"
#include "csp_qfifo.h"
#include "csp_port.h"

csp_conf_t csp_conf = {
	.version = 2,
	.address = 1,
	.hostname = "",
	.model = "",
	.revision = "",
	.conn_dfl_so = CSP_O_NONE,
	.dedup = CSP_DEDUP_OFF};

uint16_t csp_get_address(void) {
	return csp_conf.address;
}

void csp_init(void) {

	/* make offset first time, so uptime is counted from process/task boot */
	csp_get_uptime_s();

	csp_buffer_init();
	csp_conn_init();
	csp_qfifo_init();

	/* Loopback */
	csp_iflist_add(&csp_if_lo);

	/* Register loopback route */
	csp_rtable_set(0, 14, &csp_if_lo, CSP_NO_VIA_ADDRESS);
}

void csp_free_resources(void) {

	csp_rtable_free();
}

const csp_conf_t * csp_get_conf(void) {
	return &csp_conf;
}
