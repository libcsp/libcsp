#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <csp/csp.h>

void csp_id_prepend(csp_packet_t * packet);
int csp_id_strip(csp_packet_t * packet);
int csp_id_setup_rx(csp_packet_t * packet);
unsigned int csp_id_get_host_bits(void);
unsigned int csp_id_get_max_nodeid(void);
unsigned int csp_id_get_max_port(void);

int csp_id_is_broadcast(uint16_t addr, csp_iface_t * iface);

int csp_id_get_header_size(void);

#if (CSP_FIXUP_V1_ZMQ_LITTLE_ENDIAN)
void csp_id_prepend_fixup_cspv1(csp_packet_t * packet);
int csp_id_strip_fixup_cspv1(csp_packet_t * packet);
#else
static inline void csp_id_prepend_fixup_cspv1(csp_packet_t * packet) {
	csp_id_prepend(packet);
}
static inline int csp_id_strip_fixup_cspv1(csp_packet_t * packet) {
	return csp_id_strip(packet);
}
#endif

#ifdef __cplusplus
}
#endif
