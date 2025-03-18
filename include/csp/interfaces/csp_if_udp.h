/****************************************************************************
 * **File:** csp/interfaces/csp_if_udp.h
 *
 * **Description:** UDP interface.
 ****************************************************************************/
#pragma once

#include <csp/csp.h>

#include <pthread.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

	/* Should be set before calling if_udp_init */
	char * host;
	int lport;
	int rport;

	/* Internal parameters */
	pthread_t server_handle;
	struct sockaddr_in peer_addr;

	int sockfd;
} csp_if_udp_conf_t;

/**
 * @brief Initialize a UDP interface for CSP.
 *
 * Setup UDP peer
 *
 * RX task:
 *   A server task will attempt at binding to ip 0.0.0.0 port 9600
 *   If this fails, it is because another udp server is already running.
 *   The server task will continue attempting the bind and will not exit before the application is closed.
 *
 * TX peer:
 *   Outgoing CSP packets will be transferred to the peer specified by the host argument
 *
 * @param[in] iface Pointer to the CSP interface structure to be initialized.
 * @param[in] ifconf Pointer to the UDP interface configuration structure.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_if_udp_init(csp_iface_t * iface, csp_if_udp_conf_t * ifconf);

#ifdef __cplusplus
}
#endif
