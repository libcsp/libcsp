#pragma once

#include <csp/csp_types.h>

void csp_rdp_queue_init(void);
void csp_rdp_queue_flush(csp_conn_t * conn);

int csp_rdp_queue_tx_size(void);
void csp_rdp_queue_tx_add(csp_conn_t * conn, csp_packet_t * packet);
csp_packet_t * csp_rdp_queue_tx_get(csp_conn_t * conn);

int csp_rdp_queue_rx_size(void);
void csp_rdp_queue_rx_add(csp_conn_t * conn, csp_packet_t * packet);
csp_packet_t * csp_rdp_queue_rx_get(csp_conn_t * conn);
