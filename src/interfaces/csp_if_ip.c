/**
 * CSP IP Interface 
 * Stephen Flores
 * 2019/02/09
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/csp_interface.h>
#include <csp/interfaces/csp_if_ip.h>

int csp_ip_init(csp_ip_config_t config) {
    // Configure target for TX task 
    csp_ip_tx_addr = config.tx_addr;
    csp_ip_tx_port = config.tx_ip_port;
    csp_ip_rx_port = config.rx_ip_port;

    // Create socket
    csp_if_ip_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (csp_if_ip_fd < 0) {
        csp_log_error("Failed to create IP socket");
        return CSP_ERR_DRIVER;
    }

    // Configure server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4 
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(csp_ip_rx_port);
    csp_log_info("Set server port to listen on %d\n", csp_ip_rx_port);

    // Bind the socket 
    if (bind(csp_if_ip_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        csp_log_error("Failed to bind IP socket");
        return CSP_ERR_DRIVER;
    }

    // Register the IP interface with CSP 
    csp_iflist_add(&csp_if_ip);

    // Start IP server task
    csp_thread_create(csp_ip_server, "IP_SERV", 5 * CSP_IF_IP_MAX_BUF_SIZE, NULL, 0, &csp_ip_server_handle);

    return CSP_ERR_NONE;
}

// IP interface RX definition: UNIX socket server 
CSP_DEFINE_TASK(csp_ip_server) {
    while (1) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));

        // Receive incoming packets 
        uint8_t buffer[CSP_IF_IP_MAX_BUF_SIZE];
        int recvlen = recvfrom(csp_if_ip_fd, buffer, CSP_IF_IP_MAX_BUF_SIZE, 0, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
        csp_log_info("Received data over IP interface of length %d bytes...", recvlen);

        // Verify length of incoming data 
        if (recvlen >= CSP_IF_IP_MAX_BUF_SIZE) {
            csp_log_error("Incoming data was too large for IP interface! Received %d bytes, but IP MTU is %d", recvlen, CSP_IF_IP_MAX_BUF_SIZE);
            continue;
        }

        // Verify length of incoming data against max CSP buffer size 
        if (recvlen >= csp_buffer_size()) {
            csp_log_error("Incoming data was too large for CSP! Received %d bytes, but CSP max buffer size is %d", recvlen, csp_buffer_size());
            continue;
        }

        // Create CSP packet and set data appropriately
        csp_log_info("Copying incoming data buffer into a packet...");
        if (csp_buffer_remaining() == 0) {
            csp_log_error("No more buffers, cannot receive packets!");
            continue;
        }
        csp_packet_t* packet = csp_buffer_get(recvlen);
        memcpy(&(packet->id), buffer, 4); // First four bytes are CSP header 
        memcpy(packet->data, &(buffer[4]), recvlen);
        packet->length = recvlen - 4;

        // Flip packet ID 
        packet->id.ext = csp_ntoh32(packet->id.ext);

        // Send to CSP 
        csp_new_packet(packet, &csp_if_ip, NULL);
    }
}

// IP interface TX definition: UNIX socket client 
int csp_ip_tx(csp_iface_t* interfafce, csp_packet_t* packet, uint32_t timeout) {
    // Send to outgoing address, on port CSP_IF_IP_SERVER_PORT

    csp_log_info("Creating socket...");
    int temp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (temp_socket_fd < 0) {
        csp_log_error("Failed to create IP socket");
        return CSP_ERR_DRIVER;
    }

    csp_log_info("Creating target address...");
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET; 
    target_addr.sin_port = htons(csp_ip_tx_port); 
    target_addr.sin_addr.s_addr = inet_addr(csp_ip_tx_addr);
    csp_log_info("Set target port to %d\n", target_addr.sin_port);

    // Copy packet to buffer 
    csp_log_info("Copying packet to outgoing buffer...");
    int buffer_len = sizeof(packet->id) + packet->length;
    uint8_t buffer[buffer_len];

    packet->id.ext = csp_hton32(packet->id.ext);
    memcpy(buffer, &(packet->id), sizeof(packet->id));
    memcpy(&(buffer[sizeof(packet->id)]), packet->data, packet->length);

    // Send off 
    csp_log_info("Sending over IP...");
    sendto(temp_socket_fd, buffer, buffer_len, 0, (struct sockaddr*)&target_addr, sizeof(target_addr));

    close(temp_socket_fd);
    csp_log_info("Successfully send packet over IP interface");

    csp_buffer_free(packet);

    return CSP_ERR_NONE;
}

// Interface definition 
csp_iface_t csp_if_ip = {
    .name = "IP",
    .nexthop = csp_ip_tx,
    .mtu = CSP_IF_IP_MAX_BUF_SIZE,
};