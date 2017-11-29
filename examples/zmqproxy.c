#include <zmq.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <unistd.h>
#include <csp/csp.h>

static void * task_capture(void *ctx) {

    /* Subscriber (RX) */
    void *subscriber = zmq_socket(ctx, ZMQ_SUB);
    assert(zmq_connect(subscriber, "tcp://localhost:7000") == 0);
    assert(zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0) == 0);

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

        /* Create new csp packet */
        csp_packet_t * packet = malloc(1024);
        if (packet == NULL) {
            zmq_msg_close(&msg);
            continue;
        }

        /* Copy the data from zmq to csp */
        char * satidptr = ((char *) &packet->id) - 1;
        memcpy(satidptr, zmq_msg_data(&msg), datalen);
        packet->length = datalen - 4 - 1;

        printf("Input: Src %u, Dst %u, Dport %u, Sport %u, Pri %u, Flags 0x%02X, Size %"PRIu16"\r\n",
               packet->id.src, packet->id.dst, packet->id.dport,
               packet->id.sport, packet->id.pri, packet->id.flags, packet->length);

        free(packet);
        zmq_msg_close(&msg);
    }
}

int main(int argc, char ** argv) {

    /**
     * ZMQ PROXY
     */
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

    printf("Starting ZMQproxy\r\n");
    zmq_proxy(frontend, backend, NULL);

    printf("Closing ZMQproxy\r\n");
    zmq_ctx_destroy(ctx);
    return 0;

}
