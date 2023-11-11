/****************************************************************************
 * **File:** csp/interfaces/csp_if_zmqhub.h
 *
 * **Description:** ZMQ (ZeroMQ) interface.
 *
 * The ZMQ interface is designed to connect to a ZMQ hub, also refered to as
 * zmqproxy. The zmqproxy can be found under examples, and is based on
 * zmq_proxy() - provided by the ZMQ API.
 *
 * For further details on ZMQ, see http://www.zeromq.org.
 ****************************************************************************/
#pragma once

#include <csp/csp_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/** max payload data, see documentation */
#define CSP_ZMQ_MTU 2048

/**
 * zmqproxy default subscribe (rx) port.
 * The client must connect it's publish endpoint to the zmqproxy's subscribe port.
 */
#define CSP_ZMQPROXY_SUBSCRIBE_PORT   6000

/**
 * zmqproxy default publish (tx) port.
 * The client must connect it's subscribe endpoint to the zmqproxy's publish port.
 */
#define CSP_ZMQPROXY_PUBLISH_PORT     7000

/**
 * Default ZMQ interface name.
 */
#define CSP_ZMQHUB_IF_NAME            "ZMQHUB"

/**
 * Format endpoint connection string for ZMQ.
 *
 * @param[in] host host name of IP.
 * @param[in] port IP port.
 * @param[out] buf user allocated buffer for receiving formatted string.
 * @param[in] buf_size size of buf.
 * @return #CSP_ERR_NONE on succcess. #CSP_ERR_NOMEM if supplied buffer too small.
 */
int csp_zmqhub_make_endpoint(const char * host, uint16_t port, char * buf, size_t buf_size);

/**
 * Setup ZMQ interface.
 *
 * @param[in] addr only receive messages matching this address (255 means all).
 * 			       This is set as rx_filter in call to
 * @param[in] host host name or IP of zmqproxy host. Endpoints are
 * 				   created using the host and the default subscribe/publish ports.
 * @param[in] flags flags for controlling features on the connection.
 * @param[out] return_interface created CSP interface.
 * @return #CSP_ERR_NONE on succcess - else assert.
 */
int csp_zmqhub_init(uint16_t addr,
					const char * host,
					uint32_t flags,
					csp_iface_t ** return_interface);

/**
 * Setup ZMQ interface.
 *
 * @param[in] addr only receive messages matching this address (255 means all).
 * 						  This is set as a \a rx_filter.
 * @param[in] publish_endpoint publish (tx) endpoint -> connect to
 * 										  zmqproxy's subscribe port #CSP_ZMQPROXY_SUBSCRIBE_PORT.
 * @param[in] subscribe_endpoint subscribe (rx) endpoint -> connect
 * 											to zmqproxy's publish port #CSP_ZMQPROXY_PUBLISH_PORT.
 * @param[in] flags flags for controlling features on the connection.
 * @param[out] return_interface created CSP interface.
 * @return #CSP_ERR_NONE on succcess - else assert.
 */
int csp_zmqhub_init_w_endpoints(uint16_t addr,
								const char * publish_endpoint,
								const char * subscribe_endpoint,
								uint32_t flags,
								csp_iface_t ** return_interface);

/**
 * Setup ZMQ interface.
 *
 * @param[in] ifname Name of CSP interface, use NULL for default
 * 								name #CSP_ZMQHUB_IF_NAME.
 * @param[in] addr Address assigned to the CSP interface.
 *	rx_filter (const uint16_t) [in]: Rx filters, use NULL for no filters - receive all messages.
 *	rx_filter_count (unsigned int) [in]: Number of Rx filters in \a rx_filter.
 * @param[in] publish_endpoint publish (tx) endpoint -> connect to
 * 										zmqproxy's subscribe port #CSP_ZMQPROXY_SUBSCRIBE_PORT.
 * @param[in] subscribe_endpoint subscribe (rx) endpoint -> connect to zmqproxy's
 * 										publish port #CSP_ZMQPROXY_PUBLISH_PORT.
 * @param[in] flags flags for controlling features on the connection.
 * @param[out] return_interface created CSP interface.
 * @return #CSP_ERR_NONE on succcess - else assert.
 */
int csp_zmqhub_init_w_name_endpoints_rxfilter(const char * ifname, uint16_t addr,
											  const uint16_t rx_filter[], unsigned int rx_filter_count,
											  const char * publish_endpoint,
											  const char * subscribe_endpoint,
											  uint32_t flags,
											  csp_iface_t ** return_interface);


/**
 * Setup filtered ZMQ interface.
 * The filter can be enabled with promisc = 0, or disabled with promisc = 1.
 *
 * The filter only works with CSP 2.0 protocol, because the first two bytes are the
 * priority and the destination address. ZMQ does not support masking, so the code
 * actually subscribes to the address once for each priority. It also calculates
 * the broadcast address and subscribes to that, aagin for each priority. Finally the
 * global broadcast address is also subscribed to. meaning a total of 3 * 4 filters.
 *
 * If a secret key curve zmq is enabled
 */

int csp_zmqhub_init_filter2(const char * ifname, const char * host, uint16_t addr, uint16_t netmask, int promisc, csp_iface_t ** return_interface, char * sec_key, uint16_t subport, uint16_t pubport);


#ifdef __cplusplus
}
#endif
