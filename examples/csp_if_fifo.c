/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>

#define TYPE_SERVER 1
#define TYPE_CLIENT 2
#define PORT        10
#define BUF_SIZE    250

pthread_t rx_thread;
int rx_channel, tx_channel;

int csp_fifo_tx(csp_iface_t *ifc, csp_packet_t *packet, uint32_t timeout);

csp_iface_t csp_if_fifo = {
    .name = "fifo",
    .nexthop = csp_fifo_tx,
    .mtu = BUF_SIZE,
};

int csp_fifo_tx(csp_iface_t *ifc, csp_packet_t *packet, uint32_t timeout) {
    /* Write packet to fifo */
    if (write(tx_channel, &packet->length, packet->length + sizeof(uint32_t) + sizeof(uint16_t)) < 0)
        printf("Failed to write frame\r\n");
    csp_buffer_free(packet);
    return CSP_ERR_NONE;
}

void * fifo_rx(void * parameters) {
    csp_packet_t *buf = csp_buffer_get(BUF_SIZE);
    /* Wait for packet on fifo */
    while (read(rx_channel, &buf->length, BUF_SIZE) > 0) {
        csp_new_packet(buf, &csp_if_fifo, NULL);
        buf = csp_buffer_get(BUF_SIZE);
    }

    return NULL;
}

int main(int argc, char **argv) {

    int me, other, type;
    char *message = "Testing CSP", *rx_channel_name, *tx_channel_name;
    csp_socket_t *sock;
    csp_conn_t *conn;
    csp_packet_t *packet;

    /* Run as either server or client */
    if (argc != 2) {
        printf("usage: %s <server/client>\r\n", argv[0]);
        return -1;
    }

    /* Set type */
    if (strcmp(argv[1], "server") == 0) {
        me = 1;
        other = 2;
        tx_channel_name = "server_to_client";
        rx_channel_name = "client_to_server";
        type = TYPE_SERVER;
    } else if (strcmp(argv[1], "client") == 0) {
        me = 2;
        other = 1;
        tx_channel_name = "client_to_server";
        rx_channel_name = "server_to_client";
        type = TYPE_CLIENT;
    } else {
        printf("Invalid type. Must be either 'server' or 'client'\r\n");
        return -1;
    }

    /* Init CSP and CSP buffer system */
    if (csp_init(me) != CSP_ERR_NONE || csp_buffer_init(10, 300) != CSP_ERR_NONE) {
        printf("Failed to init CSP\r\n");
        return -1;
    }

    tx_channel = open(tx_channel_name, O_RDWR);
    if (tx_channel < 0) {
        printf("Failed to open TX channel\r\n");
        return -1;
    }

    rx_channel = open(rx_channel_name, O_RDWR);
    if (rx_channel < 0) {
        printf("Failed to open RX channel\r\n");
        return -1;
    }

    /* Start fifo RX task */
	pthread_create(&rx_thread, NULL, fifo_rx, NULL);

    /* Set default route and start router */
    csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_fifo, CSP_NODE_MAC);
    csp_route_start_task(0, 0);

    /* Create socket and listen for incoming connections */
    if (type == TYPE_SERVER) {
        sock = csp_socket(CSP_SO_NONE);
        csp_bind(sock, PORT);
        csp_listen(sock, 5);
    }

    /* Super loop */
    while (1) {
        if (type == TYPE_SERVER) {
            /* Process incoming packet */
            conn = csp_accept(sock, 1000);
            if (conn) {
                packet = csp_read(conn, 0);
                if (packet)
                    printf("Received: %s\r\n", packet->data);
                csp_buffer_free(packet);
                csp_close(conn);
            }
        } else {
            /* Send a new packet */
            packet = csp_buffer_get(strlen(message));
            if (packet) {
                strcpy((char *) packet->data, message);
                packet->length = strlen(message);

                conn = csp_connect(CSP_PRIO_NORM, other, PORT, 1000, CSP_O_NONE);
                printf("Sending: %s\r\n", message);
                if (!conn || !csp_send(conn, packet, 1000))
                    return -1;
                csp_close(conn);
            }
            sleep(1);
        }
    }

    close(rx_channel);
    close(tx_channel);

    return 0;
}
