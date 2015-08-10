#ifndef CSP_IF_ZMQHUB_H_
#define CSP_IF_ZMQHUB_H_

#include <csp/csp.h>

extern csp_iface_t csp_if_zmqhub;

/**
 * Setup ZMQ interface
 * @param addr only receive messages matching this address (255 means all)
 * @param host Pointer to string containing zmqproxy host
 * @return CSP_ERR
 */
int csp_zmqhub_init(char addr, char * host);

/**
 * Setup ZMQ interface
 * @param addr only receive messages matching this address (255 means all)
 * @param publisher_endpoint Pointer to string containing zmqproxy publisher endpoint
 * @param subscriber_endpoint Pointer to string containing zmqproxy subscriber endpoint
 * @return CSP_ERR
 */
int csp_zmqhub_init_w_endpoints(char _addr, char * publisher_url,
		char * subscriber_url);

#endif /* CSP_IF_ZMQHUB_H_ */
