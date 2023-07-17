#pragma once

#include <csp/interfaces/csp_if_eth.h>

void csp_if_eth_init(csp_iface_t * iface, const char * device, const char * ifname, int mtu, bool promisc);

void * csp_if_eth_rx_loop(void * param);
