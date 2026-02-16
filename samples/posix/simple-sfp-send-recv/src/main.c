/* needed for pthread_timedjoin_np */
#define _GNU_SOURCE 

#include <string.h>

#include <csp/csp.h>
#include <csp/csp_buffer.h>
#include <csp/csp_sfp.h>
#include <csp/csp_crc32.h>
#include <csp/interfaces/csp_if_lo.h>
#include <stdio.h> 
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#define RECEIVER_ADDRESS 0
#define RECEIVER_PORT    10
#define CONN_OPTS        CSP_O_CRC32
#define TIMEOUT          10000  /* in ms */
#define perr(fmt, ...)   do { fprintf(stderr, "line %d: " fmt "\n",  __LINE__, ##__VA_ARGS__); fflush(stderr); } while (0)

typedef struct {
    FILE * handle;
    char * name;
    off_t size;
} sfp_file_t;

static sfp_file_t send_file;
static sfp_file_t recv_file;

static void * router(void * params) {
    (void)params;
    
    while (1) {
        (void)csp_route_work();
    }

    return NULL;
}

static int read_from_file(uint8_t * buffer, uint32_t size, uint32_t offset, void * data) {
    (void)offset;
    sfp_file_t * file = (sfp_file_t *)data;
    if (size == fread(buffer, 1, size, file->handle))
        return CSP_ERR_NONE; 
    return CSP_ERR_SFP;
}

static int write_to_file(const uint8_t * buffer, uint32_t size, uint32_t offset, uint32_t totalsz, void * data) {
    (void)offset;
    (void)totalsz;
    sfp_file_t * file = (sfp_file_t *)data;
    if (size == fwrite(buffer, 1, size, file->handle))
        return CSP_ERR_NONE;
    return CSP_ERR_SFP;
}

static void * sender(void * params) {
    csp_conn_t * conn = NULL;
    sfp_file_t * file = (sfp_file_t *)params;

    /* Connect to receiver */
    conn = csp_connect(CSP_PRIO_NORM, RECEIVER_ADDRESS, RECEIVER_PORT, TIMEOUT, CONN_OPTS);
    if (conn) {
        csp_sfp_read_t user;
        user.data = file;
        user.read = read_from_file;

        /* Send data */
        if (CSP_ERR_NONE != csp_sfp_send(conn, &user, file->size, csp_sfp_conn_max_mtu(conn), 0)) {
            perr("Failed csp_sfp_send");
        }
    } else {
        perr("Failed csp_connect");
    }

    csp_close(conn);

    return NULL;
}

static void * receiver(void * params) {
    csp_conn_t * conn = NULL;
    sfp_file_t * file = (sfp_file_t *)params;

    csp_socket_t sock = {0};
    sock.opts |= CONN_OPTS;
    csp_bind(&sock, RECEIVER_PORT);
    csp_listen(&sock, 0);

    conn = csp_accept(&sock, TIMEOUT);
    if (conn) {
        csp_sfp_recv_t user;
        user.data = file;
        user.write = write_to_file;

        /* Receive data */
        if (CSP_ERR_NONE != csp_sfp_recv(conn, &user, 1000)) {
            perr("Failed csp_sfp_recv");
        }
    } else {
        perr("Failed csp_accept");
    }

    csp_close(conn);

    return NULL;
}

static int loopback_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {
    /* add some sleep to avoid starving the system when RDP is not used */
    if ((CONN_OPTS & CSP_O_RDP) == 0)
        usleep(1000);
        
    csp_qfifo_write(packet, &csp_if_lo, NULL);
    return CSP_ERR_NONE;
}

int main(int argc, char * argv[]) {
    /* open existing file */
    send_file.handle = fopen(argv[1], "rb");
    if (!send_file.handle) {
        csp_print("Failed to open file %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* get file size */
    if (fseek(send_file.handle, 0L, SEEK_END)) {
        csp_print("Failed to get file size %s\n", argv[1]);
        fclose(send_file.handle);
        exit(EXIT_FAILURE);
    }

    send_file.size = ftell(send_file.handle);
    rewind(send_file.handle);

    /* create file */
    recv_file.handle = fopen(argv[2], "wb");
    if (!send_file.handle) {
        csp_print("Failed to open file %s\n", argv[2]);
        fclose(send_file.handle);
        exit(EXIT_FAILURE);
    }

    /* init csp */
    csp_init();

    csp_if_lo.is_default = 1;
    csp_if_lo.nexthop = loopback_tx; /* replace default tx function

    /* start router */
    pthread_t router_thread;
    if (0 != pthread_create(&router_thread, NULL, router, NULL)) {
        perr("Failed to create thread");
        goto error;
    }

    /* start receiver */
    pthread_t receiver_thread;
    if (0 != pthread_create(&receiver_thread, NULL, receiver, &recv_file)) {
        perr("Failed to create thread");
        goto error;
    }

    /* start sender */
    pthread_t sender_thread;
    if (0 != pthread_create(&sender_thread, NULL, sender, &send_file)) {
        perr("Failed to create thread");
        goto error;
    }

    /* wait for sender to finish */
    struct timespec timeout;
    if (0 != clock_gettime(CLOCK_REALTIME, &timeout)) {
        perr("Failed to get time");
        goto error;
    }
    timeout.tv_sec += 120;
    timeout.tv_nsec = 0;
    if (0 != pthread_timedjoin_np(sender_thread, NULL, &timeout)) {
        perr("Sender timeout");
        goto error;
    }

    /* wait for receiver to finish */
    if (0 != clock_gettime(CLOCK_REALTIME, &timeout)) {
        perr("Failed to get time");
        goto error;
    }
    timeout.tv_sec += 3;
    timeout.tv_nsec = 0;
    if (0 != pthread_timedjoin_np(receiver_thread, NULL, &timeout)) {
        perr("Receiver timeout");
        goto error;
    }

    printf("Test completed!\n");
    fclose(send_file.handle);
    fclose(recv_file.handle);
    exit(EXIT_SUCCESS);

error:
    fclose(send_file.handle);
    fclose(recv_file.handle);
    exit(EXIT_FAILURE); 
}
