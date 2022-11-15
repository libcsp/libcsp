#include <csp/interfaces/csp_if_lo.h>
#include <csp/arch/csp_time.h>
#include <csp/csp_hooks.h>
#include <csp/csp_id.h>
#include "csp/autoconfig.h"
#include "csp_macro.h"
#include "csp_conn.h"
#include "csp_qfifo.h"
#include "csp_port.h"
#include "csp_rdp_queue.h"

__weak void csp_panic(const char * msg) {
	return;
}

csp_conf_t csp_conf = {
	.version = 2,
	.hostname = "",
	.model = "",
	.revision = "",
	.conn_dfl_so = CSP_O_NONE,
	.dedup = CSP_DEDUP_OFF};

void csp_init(void) {

	/* Validation of version */
	if ((csp_conf.version == 0) || (csp_conf.version > 2)) {
		csp_conf.version = 2;
	}

	/* Validation of dedup */
	if (csp_conf.dedup > CSP_DEDUP_ALL) {
		csp_conf.dedup = CSP_DEDUP_OFF;
	}

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

const csp_conf_t * csp_get_conf(void) {
	return &csp_conf;
}
