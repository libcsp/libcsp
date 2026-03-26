#include <csp/interfaces/csp_if_udp.h>

#include <csp/csp_debug.h>
#include <stdio.h>  // Required for perror
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h> // Required for strerror

#include <csp/csp.h>
#include <csp/arch/csp_endian.h>
#include <csp/csp_interface.h>
#include <csp/csp_id.h>

#ifndef MSG_CONFIRM
#define MSG_CONFIRM (0)
#endif

// Define constants to avoid magic numbers
#define CSP_UDP_RX_NOMEM_DELAY_US (10000)
#define CSP_UDP_RETRY_DELAY_S (1)

// Macro to mark unused parameters, preventing compiler warnings
#define UNUSED(x) (void)(x)

static int csp_if_udp_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {
    // Mark unused parameters
    UNUSED(via);
    UNUSED(from_me);

    csp_if_udp_conf_t * ifconf = iface->driver_data;

    if (ifconf->sockfd <= 0) { // Check for a valid socket descriptor
		csp_print(CSP_LL_ERROR, "UDP TX failed: Socket not ready\n");
		csp_buffer_free(packet);
        return CSP_ERR_DRIVER; // Return a more specific error
    }

    csp_id_prepend(packet);
    ifconf->peer_addr.sin_family = AF_INET;
    ifconf->peer_addr.sin_port = htons(ifconf->rport);

    // Check the return value of sendto
    ssize_t sent_len = sendto(ifconf->sockfd, packet->frame_begin, packet->frame_length, MSG_CONFIRM, (struct sockaddr *)&ifconf->peer_addr, sizeof(ifconf->peer_addr));
    if (sent_len != (ssize_t)packet->frame_length) {
        if (sent_len == -1) {
            perror("sendto failed");
        } else {
			csp_print(CSP_LL_ERROR, "UDP TX failed: sent %ld of %u bytes\n", sent_len, packet->frame_length);
		}
        iface->tx_error++;
    }

    csp_buffer_free(packet);
    return CSP_ERR_NONE;
}

int csp_if_udp_rx_work(int sockfd, csp_iface_t * iface) {
    csp_packet_t * packet = csp_buffer_get(0);
    if (packet == NULL) {
        return CSP_ERR_NOMEM;
    }

    /* Setup RX frame to point to ID */
    int header_size = csp_id_setup_rx(packet);
    ssize_t received_len = recvfrom(sockfd, (char *)packet->frame_begin, sizeof(packet->data) + header_size, MSG_WAITALL, NULL, NULL);

    if (received_len < header_size) {
        if (received_len == -1) {
            // Non-critical error for UDP (e.g., connection refused on other end)
            // No need to print perror here as it can be noisy
        }
        csp_buffer_free(packet);
        // Return a more appropriate error for a failed read
        return CSP_ERR_RX;
    }

    packet->frame_length = received_len;

    /* Parse the frame and strip the ID field */
    if (csp_id_strip(packet) != 0) {
        csp_buffer_free(packet);
        return CSP_ERR_INVAL;
    }

    csp_qfifo_write(packet, iface, NULL);
    return CSP_ERR_NONE;
}

void * csp_if_udp_rx_loop(void * param) {
    csp_iface_t * iface = param;
    csp_if_udp_conf_t * ifconf = iface->driver_data;

    // Loop until socket is successfully created and bound
    while (1) {
        // Use 0 for the protocol to let the system choose IPPROTO_UDP
        ifconf->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (ifconf->sockfd < 0) {
            perror("socket creation failed");
            sleep(CSP_UDP_RETRY_DELAY_S);
            continue;
        }

        struct sockaddr_in server_addr = {0};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(ifconf->lport);

        // Check the return value of bind directly
        if (bind(ifconf->sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("bind failed");
            close(ifconf->sockfd);
            ifconf->sockfd = 0; // Reset sockfd to indicate failure
			csp_print(CSP_LL_TRACE, "UDP server waiting for port %d\n", ifconf->lport);
			sleep(CSP_UDP_RETRY_DELAY_S);
            continue;
        }

		csp_print(CSP_LL_INFO, "UDP server listening on port %d\n", ifconf->lport);
		break; // Exit loop on success
    }

    // Main receive loop
    while (1) {
        int ret = csp_if_udp_rx_work(ifconf->sockfd, iface);
        if (ret == CSP_ERR_INVAL || ret == CSP_ERR_RX) {
            iface->rx_error++;
        } else if (ret == CSP_ERR_NOMEM) {
            usleep(CSP_UDP_RX_NOMEM_DELAY_US);
        }
    }

    // This code is unreachable in this design but is good practice for completeness
    close(ifconf->sockfd);
    return NULL;
}

void csp_if_udp_init(csp_iface_t * iface, csp_if_udp_conf_t * ifconf) {
    pthread_attr_t attributes;
    int ret;

    iface->driver_data = ifconf;
    ifconf->sockfd = 0; // Initialize sockfd to 0 to indicate it's not ready

    if (inet_aton(ifconf->host, &ifconf->peer_addr.sin_addr) == 0) {
		csp_print(CSP_LL_ERROR, "Unknown peer address %s\n", ifconf->host);
		// Consider returning an error here if the host is essential
    }

	csp_print(CSP_LL_INFO, "UDP peer address: %s:%d (will listen on port %d)\n", inet_ntoa(ifconf->peer_addr.sin_addr), ifconf->rport, ifconf->lport);

	/* Start server thread */
    ret = pthread_attr_init(&attributes);
    if (ret != 0) {
		csp_print(CSP_LL_ERROR, "csp_if_udp_init: pthread_attr_init failed: %s: %d\n", strerror(ret), ret);
		return; // Exit if we can't create the thread properly
    }
    ret = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    if (ret != 0) {
		csp_print(CSP_LL_ERROR, "csp_if_udp_init: pthread_attr_setdetachstate failed: %s: %d\n", strerror(ret), ret);
		pthread_attr_destroy(&attributes);
        return;
    }
    ret = pthread_create(&ifconf->server_handle, &attributes, csp_if_udp_rx_loop, iface);
    if (ret != 0) {
		csp_print(CSP_LL_ERROR, "csp_if_udp_init: pthread_create failed: %s: %d\n", strerror(ret), ret);
		pthread_attr_destroy(&attributes);
        return;
    }

    // It's safe to destroy attributes immediately after thread creation
    pthread_attr_destroy(&attributes);

    /* Register interface */
    iface->name = "UDP"; // Direct assignment is fine for string literals
    iface->nexthop = csp_if_udp_tx;
    csp_iflist_add(iface);
}
