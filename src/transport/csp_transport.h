

#pragma once

#include <csp/csp_types.h>

/** ARRIVING SEGMENT */
void csp_udp_new_packet(csp_conn_t * conn, csp_packet_t * packet);
bool csp_rdp_new_packet(csp_conn_t * conn, csp_packet_t * packet);

/** RDP: USER REQUESTS */
int csp_rdp_connect(csp_conn_t * conn);
void csp_rdp_init(csp_conn_t * conn);
int csp_rdp_close(csp_conn_t * conn, uint8_t closed_by);
void csp_rdp_conn_print(csp_conn_t * conn);
int csp_rdp_send(csp_conn_t * conn, csp_packet_t * packet);
int csp_rdp_check_ack(csp_conn_t * conn);
void csp_rdp_check_timeouts(csp_conn_t * conn);
