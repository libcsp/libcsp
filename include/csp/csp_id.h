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

#ifdef __cplusplus
}
#endif
