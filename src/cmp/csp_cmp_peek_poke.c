#include "csp_cmp_internal.h"

#include <endian.h>
#include <stdint.h>

#include <csp/csp_hooks.h>

int csp_cmp_peek_handler(csp_packet_t * packet) {

	struct csp_cmp_peek_msg * cmp = (struct csp_cmp_peek_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	cmp->addr = htobe32(cmp->addr);
	if (cmp->len > CSP_CMP_PEEK_MAX_LEN) {
		return CSP_ERR_INVAL;
	}

	int res = csp_cmp_memcpy((csp_memptr_t)(uintptr_t)cmp->data, (csp_memptr_t)(uintptr_t)cmp->addr, cmp->len);
	if (res != CSP_ERR_NONE) {
		return res;
	}

	packet->length = CMP_PEEK_SIZE(cmp->len);

	return CSP_ERR_NONE;
}

int csp_cmp_poke_handler(csp_packet_t * packet) {

	struct csp_cmp_poke_msg * cmp = (struct csp_cmp_poke_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	cmp->addr = htobe32(cmp->addr);
	if (cmp->len > CSP_CMP_POKE_MAX_LEN) {
		return CSP_ERR_INVAL;
	}

	if (csp_cmp_check_len(packet, sizeof(*cmp) + cmp->len) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	int res = csp_cmp_memcpy((csp_memptr_t)(uintptr_t)cmp->addr, (csp_memptr_t)(uintptr_t)cmp->data, cmp->len);
	if (res != CSP_ERR_NONE) {
		return res;
	}

	packet->length = CMP_POKE_SIZE(cmp->len);

	return CSP_ERR_NONE;
}

int csp_cmp_peek_v2_handler(csp_packet_t * packet) {

	struct csp_cmp_peek_v2_msg * cmp = (struct csp_cmp_peek_v2_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	cmp->vaddr = htobe64(cmp->vaddr);
	if (cmp->len > CSP_CMP_PEEK_V2_MAX_LEN) {
		return CSP_ERR_INVAL;
	}

	int res = csp_cmp_memread64(cmp->data, cmp->vaddr, cmp->len);
	if (res != CSP_ERR_NONE) {
		return res;
	}

	packet->length = CMP_PEEK_V2_SIZE(cmp->len);

	return CSP_ERR_NONE;
}

int csp_cmp_poke_v2_handler(csp_packet_t * packet) {

	struct csp_cmp_poke_v2_msg * cmp = (struct csp_cmp_poke_v2_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	cmp->vaddr = htobe64(cmp->vaddr);
	if (cmp->len > CSP_CMP_POKE_V2_MAX_LEN) {
		return CSP_ERR_INVAL;
	}

	if (csp_cmp_check_len(packet, sizeof(*cmp) + cmp->len) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	int res = csp_cmp_memwrite64(cmp->vaddr, cmp->data, cmp->len);
	if (res != CSP_ERR_NONE) {
		return res;
	}

	packet->length = CMP_POKE_V2_SIZE(cmp->len);

	return CSP_ERR_NONE;
}
