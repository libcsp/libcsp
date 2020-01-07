/**
 * Build this example on linux with:
 * ./waf configure --enable-examples --enable-if-kiss --with-driver-usart=linux --enable-crc32 clean build
 */

#include <stdio.h>
#include <csp/csp.h>
#include <csp/interfaces/csp_if_kiss.h>

#include <csp/drivers/usart.h>
#include <csp/arch/csp_thread.h>

#define PORT         10
#define MY_ADDRESS    1

#define SERVER_TIDX   0
#define CLIENT_TIDX   1
#define USART_HANDLE  0

CSP_DEFINE_TASK(task_server) {
    int running = 1;
    csp_socket_t *socket = csp_socket(CSP_SO_NONE);
    csp_conn_t *conn;
    csp_packet_t *packet;
    csp_packet_t *response;

    response = csp_buffer_get(sizeof(csp_packet_t) + 2);
    if( response == NULL ) {
        fprintf(stderr, "Could not allocate memory for response packet!\n");
        return CSP_TASK_RETURN;
    }
    response->data[0] = 'O';
    response->data[1] = 'K';
    response->length = 2;

    csp_bind(socket, CSP_ANY);
    csp_listen(socket, 5);

    printf("Server task started\r\n");

    while(running) {
        if( (conn = csp_accept(socket, 10000)) == NULL ) {
            continue;
        }

        while( (packet = csp_read(conn, 100)) != NULL ) {
            switch( csp_conn_dport(conn) ) {
                case PORT:
                    if( packet->data[0] == 'q' )
                        running = 0;
                    csp_buffer_free(packet);
                    csp_send(conn, response, 1000);
                    break;
                default:
                    csp_service_handler(conn, packet);
                    break;
            }
        }

        csp_close(conn);
    }

    csp_buffer_free(response);

    return CSP_TASK_RETURN;
}

CSP_DEFINE_TASK(task_client) {

    char outbuf = 'q';
    char inbuf[3] = {0};
    int pingResult;

    for(int i = 50; i <= 200; i+= 50) {
        pingResult = csp_ping(MY_ADDRESS, 1000, 100, CSP_O_NONE);
        printf("Ping with payload of %d bytes, took %d ms\n", i, pingResult);
        csp_sleep_ms(1000);
    }
    csp_ps(MY_ADDRESS, 1000);
    csp_sleep_ms(1000);
    csp_memfree(MY_ADDRESS, 1000);
    csp_sleep_ms(1000);
    csp_buf_free(MY_ADDRESS, 1000);
    csp_sleep_ms(1000);
    csp_uptime(MY_ADDRESS, 1000);
    csp_sleep_ms(1000);

    csp_transaction(0, MY_ADDRESS, PORT, 1000, &outbuf, 1, inbuf, 2);
    printf("Quit response from server: %s\n", inbuf);

    return CSP_TASK_RETURN;
}

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("usage: %s <uart device>\r\n", argv[0]);
        return -1;
    }

    csp_debug_toggle_level(CSP_PACKET);
    csp_debug_toggle_level(CSP_INFO);

    csp_buffer_init(10, 300);

    csp_conf_t csp_conf;
    csp_conf_get_defaults(&csp_conf);
    csp_conf.address = MY_ADDRESS;
    csp_init(&csp_conf);

    struct usart_conf conf = {
        .device = argv[1],
        .baudrate = 500000,
        .databits = 8,
        .stopbits = 1,
        .paritysetting = 0,
        .checkparity = 0
    };

#if defined(CSP_WINDOWS)
    conf.baudrate = CBR_9600;
    conf.paritysetting = NOPARITY;
    conf.stopbits = ONESTOPBIT;
    conf.checkparity = FALSE;
#elif defined(CSP_MACOSX)
    conf.baudrate = 115200;
#endif

    /* Initialize UART and register CSP KISS interface */
    csp_iface_t * csp_if_kiss;
    int res = usart_open_and_add_kiss_interface(&conf, CSP_IF_KISS_DEFAULT_NAME, &csp_if_kiss);
    if (res != CSP_ERR_NONE) {
        printf("usart_open_and_add_kiss_interface() failed, error: %d\r\n", res);
        return -1;
    }

    csp_route_set(MY_ADDRESS, csp_if_kiss, CSP_NODE_MAC);
    csp_route_start_task(0, 0);

    csp_conn_print_table();
    csp_route_print_table();
    csp_route_print_interfaces();

    csp_thread_handle_t handle_server;
    csp_thread_create(task_server, "SERVER", 1000, NULL, 0, &handle_server);
    csp_thread_handle_t handle_client;
    csp_thread_create(task_client, "CLIENT", 1000, NULL, 0, &handle_client);

    /* Wait for program to terminate (ctrl + c) */
    while(1) {
    	csp_sleep_ms(1000000);
    }

    return 0;

}
