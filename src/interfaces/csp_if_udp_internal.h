#pragma once

#include <stdint.h>

/**
 * Open a UDP socket bound to lport on every local address.
 *
 * @return the socket, or a negative errno value if it could not be created or bound
 */
int csp_if_udp_open_socket(uint16_t lport);
