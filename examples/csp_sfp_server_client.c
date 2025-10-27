/* needed for pthread_timedjoin_np */
#define _GNU_SOURCE

#include <csp/csp.h>
#include <csp/csp_buffer.h>
#include <csp/csp_sfp.h>
#include <csp/csp_crc32.h>
#include <csp/interfaces/csp_if_lo.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>

#define RECEIVER_ADDRESS 0
#define RECEIVER_PORT    10
#define TIMEOUT          10000  /* in ms */
#define perr(fmt, ...)   do { fprintf(stderr, "line %d: " fmt "\n",  __LINE__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define print(fmt, ...)  do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); fflush(stderr); } while (0)

typedef struct {
    uint32_t size;
    uint32_t crc;
} sfp_data_t;

typedef struct {
    int size;
    int timeout;
    int rdp;
    int mtu;
} test_options_t;

static test_options_t test_options = { .size = 1000000, .rdp = 0, .timeout = 30, .mtu = 128 };
static uint32_t sender_crc = 0;
static uint32_t receiver_crc = 0xffffffff;
static uint32_t received_sz = 0;

static const char * csp_error_to_str(int err) {
    switch (err) {
        case CSP_ERR_NONE:     return "No error";
        case CSP_ERR_NOMEM:    return "Not enough memory";
        case CSP_ERR_INVAL:    return "Invalid argument";
        case CSP_ERR_TIMEDOUT: return "Operation timed out";
        case CSP_ERR_USED:     return "Resource already in use";
        case CSP_ERR_NOTSUP:   return "Operation not supported";
        case CSP_ERR_BUSY:     return "Device or resource busy";
        case CSP_ERR_ALREADY:  return "Connection already in progress";
        case CSP_ERR_RESET:    return "Connection reset";
        case CSP_ERR_NOBUFS:   return "No more buffer space available";
        case CSP_ERR_TX:       return "Transmission failed";
        case CSP_ERR_DRIVER:   return "Error in driver layer";
        case CSP_ERR_AGAIN:    return "Resource temporarily unavailable";
        case CSP_ERR_NOSYS:    return "Function not implemented";
        case CSP_ERR_HMAC:     return "HMAC failed";
        case CSP_ERR_CRC32:    return "CRC32 failed";
        case CSP_ERR_SFP:      return "SFP protocol error or inconsistency";
        case CSP_ERR_MTU:      return "Invalid MTU";
        default:               return "Unknown error";
    }
}

static void print_help(const char * program_name) {
    uint32_t max_mtu_nordp = csp_sfp_opts_max_mtu(CSP_O_CRC32);
    uint32_t max_mtu_rdp = csp_sfp_opts_max_mtu(CSP_O_CRC32 | CSP_O_RDP);

    print("Usage: %s [OPTIONS]", program_name);
    print("Options:");
    print("  -r,--rdp=VALUE       Enable/Disable (1 or 0) RDP mode (optional).");
    print("                       Default is %s", test_options.rdp ? "Enabled" : "Disabled");
    print("  -t,--timeout=SECONDS Set the timeout in seconds (optional).");
    print("                       Values above INT_MAX might crash the program.");
    print("                       Default is %d seconds", test_options.timeout);
    print("  -s,--size=BYTES      Set the total transfer size in bytes (optional).");
    print("                       Values above INT_MAX might crash the program");
    print("                       Default is %d bytes", test_options.size);
    print("  -m,--mtu=BYTES       Set the maximum transfer unit in bytes (optional).");
    print("                       Values above INT_MAX might crash the program");
    print("                       Default is %d bytes", test_options.mtu);
    print("                       Max mtu with RDP disabled is %d bytes", max_mtu_nordp);
    print("                       Max mtu with RDP enabled is %d bytes", max_mtu_rdp);
    print("  -h,--help            Show this help message.");
}

static void process_args(int argc, char * argv[], test_options_t * test_opts) {
    static struct option long_options[] = {
        { "rdp",     required_argument, NULL, 'r' },
        { "timeout", required_argument, NULL, 't' },
        { "size",    required_argument, NULL, 's' },
        { "mtu",     required_argument, NULL, 'm' },
        { "help",    no_argument,       NULL, 'h' },
        { NULL,      no_argument,       NULL, 0   }  /* Terminator */
    };

    int opt;

    while ((opt = getopt_long(argc, argv, "r:t:s:m:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'r':
                test_opts->rdp = atoi(optarg);
                if (test_opts->rdp < 0) {
                    perr("'rdp' must be non-negative.");
                    exit(EXIT_FAILURE);
                }
                break;

            case 't':
                test_opts->timeout = atoi(optarg);
                if (test_opts->timeout < 0) {
                    perr("'timeout' must be non-negative.");
                    exit(EXIT_FAILURE);
                }
                break;

            case 's':
                test_opts->size = atoi(optarg);
                if (test_opts->size < 0) {
                    perr("'size' must be non-negative.");
                    exit(EXIT_FAILURE);
                }
                break;

            case 'm':
                test_opts->mtu = atoi(optarg);
                if (test_opts->mtu < 0) {
                    perr("'mtu' must be non-negative.");
                    exit(EXIT_FAILURE);
                }
                break;

            case 'h':
                print_help(argv[0]);
                exit(EXIT_SUCCESS);

            default: /* Invalid option */
                print_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

static int read_from_buffer(uint8_t * buffer, uint32_t size, uint32_t offset, void * data) {
    (void)offset;

    sfp_data_t * d = (sfp_data_t *)data;

    /* Seed the random number generator */
    srand(time(NULL));

    /* Fill the array with random values */
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)(rand() % 256); /* Assign a random number to each element */
    }

    /* update crc */
    csp_crc32_update(&d->crc, buffer, size);

    return CSP_ERR_NONE;
}

static int write_to_buffer(const uint8_t * buffer, uint32_t size, uint32_t offset, uint32_t totalsz, void * data) {
    (void)offset;
    (void)totalsz;

    sfp_data_t * d = (sfp_data_t *)data;

    /* update crc */
    csp_crc32_update(&d->crc, buffer, size);

    /* update counter */
    received_sz += size;

    return CSP_ERR_NONE;
}

static void * router(void * params) {
    (void)params;

    while (1) {
        (void)csp_route_work();
    }

    return NULL;
}

static void * sender(void * params) {
    const test_options_t * test_opts = (test_options_t *)params;
    sfp_data_t data;
    csp_crc32_init(&data.crc);
    csp_conn_t * conn = NULL;
    uint32_t opts = CSP_O_CRC32;
    opts |= test_opts->rdp ? CSP_O_RDP : 0;

    /* Connect to receiver */
    conn = csp_connect(CSP_PRIO_NORM, RECEIVER_ADDRESS, RECEIVER_PORT, TIMEOUT, opts);
    if (NULL == conn) {
        perr("Failed csp_connect");
        goto exit;
    }

    csp_sfp_read_t user;
    user.data = &data;
    user.read = read_from_buffer;

    /* Send data */
    int ret = csp_sfp_send(conn, &user, test_opts->size, test_opts->mtu, 0);
    if (CSP_ERR_NONE != ret) {
        perr("Failed csp_sfp_send dew to: %s", csp_error_to_str(ret));
        goto exit;
    }

    sender_crc = csp_crc32_final(&data.crc);
exit:
    csp_close(conn);

    return NULL;
}

static void * receiver(void * params) {
    const test_options_t * test_opts = (test_options_t *)params;
    sfp_data_t data;
    csp_crc32_init(&data.crc);
    csp_conn_t * conn = NULL;

    csp_socket_t sock = {0};
    sock.opts |= CSP_SO_CRC32REQ;
    sock.opts |= test_opts->rdp ? CSP_SO_RDPREQ : 0;
    csp_listen(&sock, 0);
    csp_bind(&sock, CSP_ANY);

    conn = csp_accept(&sock, TIMEOUT);
    if (!conn) {
        perr("Failed csp_accept");
        goto exit;
    }

    csp_sfp_recv_t user;
    user.data = &data;
    user.write = write_to_buffer;

    /* Send data */
    int ret = csp_sfp_recv(conn, &user, 1000);
    if (CSP_ERR_NONE != ret) {
        perr("Failed csp_sfp_recv dew to: %s", csp_error_to_str(ret));
        goto exit;
    }

    receiver_crc = csp_crc32_final(&data.crc);
exit:
    csp_close(conn);

    return NULL;
}

static int loopback_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {
    (void)iface;
    (void)via;
    (void)from_me;

    /* add some sleep to avoid starving the system when RDP is not used */
    if (!test_options.rdp)
        usleep(1000);
    csp_qfifo_write(packet, &csp_if_lo, NULL);
    return CSP_ERR_NONE;
}

int main(int argc, char * argv[]) {
    process_args(argc, argv, &test_options);
    csp_init();

    csp_if_lo.is_default = 1;
    csp_if_lo.nexthop = loopback_tx; /* replace default tx function */

    pthread_t router_thread;
    if (0 != pthread_create(&router_thread, NULL, router, NULL)) {
        perr("Failed to create thread");
        exit(EXIT_FAILURE);
    }

    pthread_t receiver_thread;
    if (0 != pthread_create(&receiver_thread, NULL, receiver, &test_options)) {
        perr("Failed to create thread");
        exit(EXIT_FAILURE);
    }

    pthread_t sender_thread;
    if (0 != pthread_create(&sender_thread, NULL, sender, &test_options)) {
        perr("Failed to create thread");
        exit(EXIT_FAILURE);
    }

    /* wait for sender to finish */
    struct timespec timeout;
    if (0 != clock_gettime(CLOCK_REALTIME, &timeout)) {
        perr("Failed to get time");
        exit(EXIT_FAILURE);
    }
    timeout.tv_sec += test_options.timeout;
    timeout.tv_nsec = 0;
    if (0 != pthread_timedjoin_np(sender_thread, NULL, &timeout)) {
        perr("Sender timeout");
        exit(EXIT_FAILURE);
    }

    /* wait for receiver to finish */
    if (0 != clock_gettime(CLOCK_REALTIME, &timeout)) {
        perr("Failed to get time");
        exit(EXIT_FAILURE);
    }
    timeout.tv_sec += 2;
    timeout.tv_nsec = 0;
    if (0 != pthread_timedjoin_np(receiver_thread, NULL, &timeout)) {
        perr("Receiver timeout");
        exit(EXIT_FAILURE);
    }

    /* compare calculated CRCs */
    if (sender_crc != receiver_crc) {
        perr("CRC mismatch");
        exit(EXIT_FAILURE);
    }

    if ((uint32_t)test_options.size != received_sz) {
        perr("SIZE mismatch");
        exit(EXIT_FAILURE);
    }

    printf("Test completed!\n");

    exit(EXIT_SUCCESS);
}
