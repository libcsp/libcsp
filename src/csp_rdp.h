#pragma once

#include <csp/csp_types.h>

bool csp_rdp_new_packet(csp_conn_t * conn, csp_packet_t * packet);

void csp_rdp_init(csp_conn_t * conn);
void csp_rdp_check_timeouts(csp_conn_t * conn);
int csp_rdp_connect(csp_conn_t * conn);
int csp_rdp_close(csp_conn_t * conn, uint8_t closed_by);
int csp_rdp_send(csp_conn_t * conn, csp_packet_t * packet);
int csp_rdp_check_ack(csp_conn_t * conn);
bool csp_rdp_conn_is_active(csp_conn_t *conn);
