/* This example was tested using two usb2can connected in loopback, in posix.*/

#include <unistd.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>
#include <csp/arch/csp_clock.h>
#include <csp/interfaces/csp_if_can.h>

#define TYPE_SERVER 1
#define TYPE_CLIENT 2
#define PORT        10
#define BUF_SIZE    250

char *message0 = "Testing CSP0";
char *message1 = "Testing CSP1";
int me, other, type;
static csp_iface_t csp_if_can0;
static csp_iface_t csp_if_can1;

void _send(csp_packet_t *packet, csp_iface_t *ifc, char * message)
{
    printf("Sending: %s, with %s\r\n", message, ifc->name);
    strcpy((char *) packet->data, message);
    packet->length = strlen(message);
    csp_route_set(other, ifc, CSP_NODE_MAC);
    csp_sendto(CSP_PRIO_NORM, other, PORT, PORT, CSP_SO_NONE, packet, 1000);
}

void client_server_pseudo_task(void)
{
    csp_socket_t *sock;
    csp_packet_t *packet0;
    csp_packet_t *packet1;
    csp_packet_t *packet;
    bool toggle_interface = true;


    if (type == TYPE_SERVER) {
        sock = csp_socket(CSP_SO_CONN_LESS);
        csp_bind(sock, PORT);
    }


    for (;;) {

        if (type == TYPE_CLIENT) {
                toggle_interface = false;
                packet0 = csp_buffer_get(strlen(message0));
                if (packet0) {
                    _send(packet0, &csp_if_can0, message0);
                }
                toggle_interface = true;
                packet1 = csp_buffer_get(strlen(message1));
                if (packet1) {
                    _send(packet1, &csp_if_can1, message1);
                }
            sleep(1);
        } else {
            packet = csp_recvfrom(sock, 1000);
            if (packet) {
                printf("Received: %s\r\n", packet->data);
                csp_buffer_free(packet);
            }
        }
    }
}

int main(int argc, char **argv)
{
    static unsigned char ucParameterToPass;
    struct csp_can_config can_conf0 = {.ifc = "can0"};
    struct csp_can_config can_conf1 = {.ifc = "can1"};

    if (argc != 2) {
        printf("usage: %s <server/client>\r\n", argv[0]);
        return -1;
    }

    /* Set type */
    if (strcmp(argv[1], "server") == 0) {
        me = 1;
        other = 2;
        type = TYPE_SERVER;
    } else if (strcmp(argv[1], "client") == 0) {
        me = 2;
        other = 1;
        type = TYPE_CLIENT;
    } else {
        printf("Invalid type. Must be either 'server' or 'client'\r\n");
        return -1;
    }

    /* Init CSP and CSP buffer system */
    if (csp_init(me) != CSP_ERR_NONE || csp_buffer_init(10, 300) != CSP_ERR_NONE) {
        return;
    }


    /* last two arguments only matters if first argument is true (single_interface) */
    csp_can_init(false, 0, NULL);

    csp_can_init_ifc(&csp_if_can0, CSP_CAN_MASKED, &can_conf0);
    csp_can_init_ifc(&csp_if_can1, CSP_CAN_MASKED, &can_conf1);

    csp_route_start_task(0, 0);

    client_server_pseudo_task();
    return 0;
}
