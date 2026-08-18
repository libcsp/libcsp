#pragma once

#include <csp/csp.h>
#include <csp/csp_cmp.h>

#include <stddef.h>
static inline int csp_cmp_check_len(const csp_packet_t * packet, size_t min_len) {

	if (packet->length < min_len) {
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}

static inline int csp_cmp_check_crc32(const csp_packet_t * packet) {

	return ((csp_conf.version == 1) || (packet->id.flags & CSP_FCRC32)) ? CSP_ERR_NONE : CSP_ERR_CRC32;
}

int csp_cmp_handler(csp_packet_t * packet);

int csp_cmp_ident_handler(csp_packet_t * packet);
int csp_cmp_route_set_v1_handler(csp_packet_t * packet);
int csp_cmp_route_set_v2_handler(csp_packet_t * packet);
int csp_cmp_if_stats_handler(csp_packet_t * packet);
int csp_cmp_peek_handler(csp_packet_t * packet);
int csp_cmp_poke_handler(csp_packet_t * packet);
int csp_cmp_peek_v2_handler(csp_packet_t * packet);
int csp_cmp_poke_v2_handler(csp_packet_t * packet);
int csp_cmp_clock_handler(csp_packet_t * packet);
