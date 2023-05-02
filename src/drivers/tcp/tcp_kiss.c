#include <csp/interfaces/csp_if_kiss.h>
#include <csp/csp_debug.h>
#include <csp/csp.h>

#include <csp/interfaces/csp_if_kiss.h>
#include <csp/drivers/tcp_kiss.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>


#define LOCALHOST "127.0.0.1"
#define TEST_PORT 8080

int csp_tcp_open(const csp_tcp_conf_t *conf) {
    struct sockaddr_in remote = {0};         // all members set to zero
    remote.sin_addr.s_addr = inet_addr(conf->address);
    remote.sin_port = htons(conf->port);     // htons: converts port to big-endian for networking
    remote.sin_family = AF_INET;		     // address family: IPV4

    return connect(
        conf->socket, (struct sockaddr *)&remote, sizeof(struct sockaddr_in)
    );
}

int csp_tcp_send(const csp_tcp_conf_t *conf, void *user_data, int length) {
    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    // if setting the socket options fails
    if(setsockopt(conf->socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        printf("timeout!\n");
        return -1;
        // TODO: CSP Error?
    }

    return send(conf->socket, user_data, length, 0);
}

int csp_tcp_receive(const csp_tcp_conf_t *conf, void *receive_data, int length) {
    int retval = -1;

    // timeval {
    //   secondques
    //   useconds
    // }
    struct timeval tv;
    tv.tv_sec = 20;     // 20 second timeout
    tv.tv_usec = 0;

    // Enums from socket-constants.h
    if(setsockopt(conf->socket, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof(tv)) < 0)
    {
        printf("timeout!\n");
        return -1;
        // TODO: CSP Error?
    }

    // recv() is a blocking function, which will block
    // the thread until data is received
    retval = recv( conf->socket, receive_data, length, 0);

    //printf("response: %s\n", response);
    return retval;
}

static int tcp_kiss_driver_tx(void * driver_data, const unsigned char * data, size_t data_length) {

	tcp_kiss_context_t *ctx = driver_data;
	if (csp_tcp_send( &(ctx->tcp_driver), data, data_length) == (int)data_length) {
		return CSP_ERR_NONE;
	}
	return CSP_ERR_TX;
}

static void tcp_kiss_driver_rx(void * user_data, uint8_t * data, size_t data_size, void * pxTaskWoken) {

	tcp_kiss_context_t * ctx = user_data;
	csp_kiss_rx(&ctx->iface, data, data_size, NULL);
}

/**
 * @brief 
 *  I think this function builds out an iface struct
 * 
 * @param conf 
 * @param ifname 
 * @param return_iface 
 * @return int 
 */
int csp_tcp_open_and_add_kiss_interface(const csp_tcp_conf_t *conf, const char * ifname, csp_iface_t ** return_iface) {

// struct csp_iface_s {

// 	uint16_t addr;              // Host address on this subnet
// 	uint16_t netmask;           // Subnet mask
// 	const char * name;          // Name, max compare length is #CSP_IFLIST_NAME_MAX
// 	void * interface_data;      // Interface data, only known/used by the interface layer, e.g. state information.
// 	void * driver_data;         // Driver data, only known/used by the driver layer, e.g. device/channel references.
// 	nexthop_t nexthop;          // Next hop (Tx) function
// 	uint16_t mtu;               // Maximum Transmission Unit of interface
// 	uint8_t split_horizon_off;  // Disable the route-loop prevention
// 	uint32_t tx;                // Successfully transmitted packets
// 	uint32_t rx;                // Successfully received packets
// 	uint32_t tx_error;          // Transmit errors (packets)
// 	uint32_t rx_error;          // Receive errors, e.g. too large message
// 	uint32_t drop;              // Dropped packets
// 	uint32_t autherr;           // Authentication errors (packets)
// 	uint32_t frame;             // Frame format errors (packets)
// 	uint32_t txbytes;           // Transmitted bytes
// 	uint32_t rxbytes;           // Received bytes
// 	uint32_t irq;               // Interrupts
// 	struct csp_iface_s * next;  // Internal, interfaces are stored in a linked list
// };
    
	if (ifname == NULL) {
		ifname = CSP_IF_KISS_DEFAULT_NAME;
	}

	tcp_kiss_context_t* ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		return CSP_ERR_NOMEM;
	}

	strncpy(ctx->name, ifname, sizeof(ctx->name) - 1);
	ctx->iface.name = ctx->name;
	ctx->iface.driver_data = ctx;
	ctx->iface.interface_data = &ctx->ifdata;
	ctx->ifdata.tx_func = tcp_kiss_driver_tx;

	int res = csp_kiss_add_interface(&ctx->iface);
	if (res == CSP_ERR_NONE) {
        res = csp_tcp_open(&conf);
	}

    // if return_iface is not null?
	if (return_iface) {
		*return_iface = &ctx->iface;
	}

	return res;
}