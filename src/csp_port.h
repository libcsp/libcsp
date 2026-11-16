

#pragma once
#include <pthread.h>
#include <csp/csp_types.h>

csp_socket_t * csp_port_get_socket(unsigned int dport);
bool csp_port_dispatch_callback(csp_packet_t* packet, unsigned int port, uint32_t security_opts,csp_iface_t *iface, csp_security_check_cb_t validate);
bool csp_socket_is_ready_to_receive(const csp_socket_t * socket);
bool csp_socket_is_conn_less(const csp_socket_t * socket);
