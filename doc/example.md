Client and server example
=========================

The following examples show the initialization of the protocol stack and examples of client/server code.

Initialization Sequence
-----------------------

This code initializes the CSP buffer system, device drivers and router core. The example uses the CAN interface function csp_can_tx but the initialization is similar for other interfaces. The loopback interface does not require any explicit initialization.

``` c
#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

/* CAN configuration struct for SocketCAN interface "can0" */
struct csp_can_config can_conf = {.ifc = "can0"};

/* Init buffer system with 10 packets of maximum 320 bytes each */
csp_buffer_init(10, 320);

/* Init CSP with address 1 */
csp_init(1);

/* Init the CAN interface with hardware filtering */
csp_can_init(CSP_CAN_MASKED, &can_conf)

/* Setup default route to CAN interface */
csp_route_set(CSP_DEFAULT_ROUTE, &csp_can_tx, CSP_HOST_MAC);

/* Start router task with 500 word stack, OS task priority 1 */
csp_route_start_task(500, 1);
```

Server
------

This example shows how to create a server task that listens for incoming connections. CSP should be initialized before starting this task. Note the use of `csp_service_handler()` as the default branch in the port switch case. The service handler will automatically reply to ICMP-like requests, such as pings and buffer status requests.

``` c
void csp_task(void *parameters) {
    /* Create socket without any socket options */
    csp_socket_t *sock = csp_socket(CSP_SO_NONE);

    /* Bind all ports to socket */
    csp_bind(sock, CSP_ANY);

    /* Create 10 connections backlog queue */
    csp_listen(sock, 10);

    /* Pointer to current connection and packet */
    csp_conn_t *conn;
    csp_packet_t *packet;

    /* Process incoming connections */
    while (1) {
        /* Wait for connection, 10000 ms timeout */    
        if ((conn = csp_accept(sock, 10000)) == NULL)
            continue;

        /* Read packets. Timout is 1000 ms */
        while ((packet = csp_read(conn, 1000)) != NULL) {
            switch (csp_conn_dport(conn)) {
                case MY_PORT:
                    /* Process packet here */
                default:
                    /* Let the service handler reply pings, buffer use, etc. */
                    csp_service_handler(conn, packet);
                    break;
            }
        }

        /* Close current connection, and handle next */
        csp_close(conn);
    }
}
```

Client
------

This example shows how to allocate a packet buffer, connect to another host and send the packet. CSP should be initialized before calling this function. RDP, XTEA, HMAC and CRC checksums can be enabled per connection, by setting the connection option to a bitwise OR of any combination of `CSP_O_RDP`, `CSP_O_XTEA`, `CSP_O_HMAC` and `CSP_O_CRC`.

``` c
int send_packet(void) {

    /* Get packet buffer for data */
    csp_packet_t *packet = csp_buffer_get(data_size);
    if (packet == NULL) {
        /* Could not get buffer element */
        printf("Failed to get buffer element\\n");
        return -1;
    }

    /* Connect to host HOST, port PORT with regular UDP-like protocol and 1000 ms timeout */
    csp_conn_t *conn = csp_connect(CSP_PRIO_NORM, HOST, PORT, 1000, CSP_O_NONE);
    if (conn == NULL) {
        /* Connect failed */
        printf("Connection failed\\n");
        /* Remember to free packet buffer */
        csp_buffer_free(packet);
        return -1;
    }

    /* Copy message to packet */
    char *msg = "HELLO";
    strcpy(packet->data, msg);

    /* Set packet length */
    packet->length = strlen(msg);

    /* Send packet */
    if (!csp_send(conn, packet, 1000)) {
        /* Send failed */
        printf("Send failed\\n");
        csp_buffer_free(packet);
    }

    /* Close connection */
    csp_close(conn);

    return 0
}
```

