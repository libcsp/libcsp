#include <stdio.h>
#include <process.h>
#include <Windows.h>
#ifdef interface
#undef interface
#endif
#include <csp/csp.h>
#include <csp/interfaces/csp_if_kiss.h>
#include <csp/drivers/usart_windows.h>

#define PORT 10
#define MY_ADDRESS 1

#define SERVER_TIDX 0
#define CLIENT_TIDX 1

unsigned WINAPI serverTask(void *params);
unsigned WINAPI clientTask(void *params);

static HANDLE threads[2];

int main(void) {
    usart_win_conf_t settings;
    settings.intf = L"COM4";
    settings.baudrate = CBR_9600;
    settings.databits = 8;
    settings.paritysetting = NOPARITY;
    settings.stopbits = ONESTOPBIT;
    settings.checkparity = FALSE;

    csp_buffer_init(10, 300);
    csp_init(MY_ADDRESS);

    if( usart_init(&settings) ) {
        printf("Failure when initialising USART!\n");
        return 1;
    }
    csp_kiss_init(0);
    usart_listen();

    csp_route_set(MY_ADDRESS, &csp_if_kiss, CSP_NODE_MAC);
    csp_route_start_task(0, 0);

    csp_debug_toggle_level(4);
    csp_conn_print_table();
    csp_route_print_table();
    csp_route_print_interfaces();

    threads[SERVER_TIDX] = (HANDLE)_beginthreadex(NULL, 0, &serverTask, NULL, 0, NULL);
    threads[CLIENT_TIDX] = (HANDLE)_beginthreadex(NULL, 0, &clientTask, NULL, 0, NULL);

    WaitForMultipleObjects(2, threads, TRUE, INFINITE);
    
    return 0;
}


unsigned WINAPI serverTask(void *params) {
    int running = 1;
    csp_socket_t *socket = csp_socket(CSP_SO_NONE);
    csp_conn_t *conn;
    csp_packet_t *packet;
    csp_packet_t *response;

    response = csp_buffer_get(sizeof(csp_packet_t) + 2);
    if( response == NULL ) {
        fprintf(stderr, "Could not allocate memory for response packet!\n");
        return 1;
    }
    response->data[0] = 'O';
    response->data[1] = 'K';
    response->length = 2;


    csp_bind(socket, CSP_ANY);
    csp_listen(socket, 5);

    while(running) {
        if( (conn = csp_accept(socket, 10000)) == NULL ) {
            continue;
        }

        while( (packet = csp_read(conn, 100)) != NULL ) {
            switch( csp_conn_dport(conn) ) {
                case PORT:
                    printf("Received packet!\n");
                    if( packet->data[0] == 'q' )
                        running = 0;
                    csp_buffer_free(packet);
                    csp_send(conn, response, 1000);
                    break;
                default:
                    printf("Received other packet. Passing it to the service handler\n");
                    csp_service_handler(conn, packet);
                    break;
            }
        }

        csp_close(conn);
    }

    csp_buffer_free(response);

    return 0;
}

unsigned WINAPI clientTask(void *params) {
    char outbuf = 'q';
    char inbuf[3] = {0};
    int result = csp_ping(MY_ADDRESS, 10000, 100, CSP_O_NONE);
    printf("Ping result %d ms\n", result);
    /*
    Sleep(1000);
    csp_ps(MY_ADDRESS, 1000);
    Sleep(1000);
    csp_memfree(MY_ADDRESS, 1000);
    Sleep(1000);
    csp_buf_free(MY_ADDRESS, 1000);
    Sleep(1000);
    csp_uptime(MY_ADDRESS, 1000);
    Sleep(1000);
    */
    csp_transaction(0, MY_ADDRESS, PORT, 1000, &outbuf, 1, inbuf, 2);
    printf("Quit response from server: %s\n", inbuf);

    return 0;
}
