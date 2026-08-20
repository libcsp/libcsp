#pragma once

#include <stddef.h>

#include <csp/csp_interface.h>

size_t csp_zmqhub_raw_header_size(void);
size_t csp_zmqhub_max_raw_frame_length(void);
int csp_zmqhub_rx(void * subscriber, csp_iface_t * iface);
