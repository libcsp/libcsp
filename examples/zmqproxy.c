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

#include <unistd.h>
#include <stdlib.h>
#include <zmq.h>
#include <assert.h>
#include <pthread.h>

#include <csp/csp.h>

static void * task_capture(void *ctx) {

    /* Subscriber (RX) */
    void *subscriber = zmq_socket(ctx, ZMQ_SUB);
    assert(zmq_connect(subscriber, "tcp://localhost:7000") == 0);
    assert(zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0) == 0);

    /* Allocated 'raw' CSP packet */
    csp_packet_t * packet = malloc(1024);
    assert(packet != NULL);

    while (1) {
    	zmq_msg_t msg;
        zmq_msg_init_size(&msg, 1024);

        /* Receive data */
        if (zmq_msg_recv(&msg, subscriber, 0) < 0) {
            zmq_msg_close(&msg);
            csp_log_error("ZMQ: %s\r\n", zmq_strerror(zmq_errno()));
            continue;
        }

        int datalen = zmq_msg_size(&msg);
        if (datalen < 5) {
            csp_log_warn("ZMQ: Too short datalen: %u\r\n", datalen);
            while(zmq_msg_recv(&msg, subscriber, ZMQ_NOBLOCK) > 0)
                zmq_msg_close(&msg);
            continue;
        }

        /* Copy the data from zmq to csp */
        char * satidptr = ((char *) &packet->id) - 1;
        memcpy(satidptr, zmq_msg_data(&msg), datalen);
        packet->length = datalen - sizeof(packet->id) - 1;

        csp_log_packet("Input: Src %u, Dst %u, Dport %u, Sport %u, Pri %u, Flags 0x%02X, Size %"PRIu16,
                       packet->id.src, packet->id.dst, packet->id.dport,
                       packet->id.sport, packet->id.pri, packet->id.flags, packet->length);

        zmq_msg_close(&msg);
    }
}

int main(int argc, char ** argv) {

    csp_debug_level_t debug_level = CSP_PACKET;
    int opt;
    while ((opt = getopt(argc, argv, "d:h")) != -1) {
        switch (opt) {
            case 'd':
                debug_level = atoi(optarg);
                break;
            default:
                printf("Usage:\n"
                       " -d <debug-level> debug level, 0 - 6\n");
                exit(1);
                break;
        }
    }

    /* enable/disable debug levels */
    for (csp_debug_level_t i = 0; i <= CSP_LOCK; ++i) {
        csp_debug_set_level(i, (i <= debug_level) ? true : false);
    }

    void * ctx = zmq_ctx_new();
    assert(ctx);

    void *frontend = zmq_socket(ctx, ZMQ_XSUB);
    assert(frontend);
    assert(zmq_bind (frontend, "tcp://*:6000") == 0);

    void *backend = zmq_socket(ctx, ZMQ_XPUB);
    assert(backend);
    assert(zmq_bind(backend, "tcp://*:7000") == 0);

    pthread_t capworker;
    pthread_create(&capworker, NULL, task_capture, ctx);

    csp_log_info("Starting ZMQproxy");
    zmq_proxy(frontend, backend, NULL);

    csp_log_info("Closing ZMQproxy");
    zmq_ctx_destroy(ctx);

    return 0;
}
