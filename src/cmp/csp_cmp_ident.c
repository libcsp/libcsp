#include "csp_cmp_internal.h"

#include <string.h>

int csp_cmp_ident_handler(csp_packet_t * packet) {

	struct csp_cmp_ident_msg * cmp = (struct csp_cmp_ident_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	strncpy(cmp->revision, csp_conf.revision, CSP_CMP_IDENT_REV_LEN);
	cmp->revision[CSP_CMP_IDENT_REV_LEN - 1] = '\0';

#if CSP_REPRODUCIBLE_BUILDS == 0
	strncpy(cmp->date, __DATE__, CSP_CMP_IDENT_DATE_LEN);
	cmp->date[CSP_CMP_IDENT_DATE_LEN - 1] = '\0';

	strncpy(cmp->time, __TIME__, CSP_CMP_IDENT_TIME_LEN);
	cmp->time[CSP_CMP_IDENT_TIME_LEN - 1] = '\0';
#endif

	strncpy(cmp->hostname, csp_conf.hostname, CSP_HOSTNAME_LEN);
	cmp->hostname[CSP_HOSTNAME_LEN - 1] = '\0';

	strncpy(cmp->model, csp_conf.model, CSP_MODEL_LEN);
	cmp->model[CSP_MODEL_LEN - 1] = '\0';

	packet->length = sizeof(*cmp);

	return CSP_ERR_NONE;
}
