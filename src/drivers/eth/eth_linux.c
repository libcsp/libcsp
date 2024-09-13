#include <stdint.h>

#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/csp_interface.h>
#include <csp/interfaces/csp_if_eth.h>
#include <csp/interfaces/csp_if_eth_pbuf.h>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <pthread.h>

/* (netinet/ether.h) protocol setting used for promiscuous mode */
#define ETH_P_ALL	0x0003		/* Every packet (be careful!!!) */

extern bool eth_debug;

typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_eth_interface_data_t ifdata;
    int sockfd;
    struct ifreq if_idx;
} eth_context_t;

int csp_eth_tx_frame(void * driver_data, csp_eth_header_t *eth_frame) {

    const eth_context_t * ctx = (eth_context_t*)driver_data;

    /* Destination socket address */
    struct sockaddr_ll socket_address = {};
    socket_address.sll_ifindex = ctx->if_idx.ifr_ifindex;
    socket_address.sll_halen = CSP_ETH_ALEN;
    memcpy(socket_address.sll_addr, eth_frame->ether_dhost, CSP_ETH_ALEN);

    uint32_t txsize = sizeof(csp_eth_header_t) + be16toh(eth_frame->seg_size);

    if (sendto(ctx->sockfd, (void*)eth_frame, txsize, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0) {
        return CSP_ERR_DRIVER;
    }

    return CSP_ERR_NONE;
}

void * csp_eth_rx_loop(void * param) {

    eth_context_t * ctx = param;

    static uint8_t recvbuf[CSP_ETH_BUF_SIZE];
    csp_eth_header_t * eth_frame = (csp_eth_header_t *)recvbuf;

    while(1) {

        /* Receive packet segment */ 

        uint32_t received_len = recvfrom(ctx->sockfd, recvbuf, CSP_ETH_BUF_SIZE, 0, NULL, NULL);

        csp_eth_rx(&ctx->ifdata.iface, eth_frame, received_len, NULL);
    }

    return NULL;
}

static uint8_t csp_eth_tx_buffer[CSP_ETH_BUF_SIZE];

int csp_eth_init(const char * device, const char * ifname, int mtu, unsigned int node_id, bool promisc, csp_iface_t ** return_iface) {

	eth_context_t * ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		return CSP_ERR_NOMEM;
	}
	
	strcpy(ctx->name, ifname);
	ctx->ifdata.iface.name = ctx->name;
    ctx->ifdata.tx_func = &csp_eth_tx_frame;
    ctx->ifdata.tx_buf = (csp_eth_header_t*)&csp_eth_tx_buffer;
    ctx->ifdata.iface.nexthop = &csp_eth_tx,
	ctx->ifdata.iface.addr = node_id;
	ctx->ifdata.iface.driver_data = ctx;
    ctx->ifdata.iface.interface_data = &ctx->ifdata;
    ctx->ifdata.promisc = promisc;

    /* Ether header 14 byte, seg header 4 byte, CSP header 6 byte */
    if (mtu < 24) {
        csp_print("csp_if_eth_init: mtu < 24\n");
        return CSP_ERR_INVAL;
    }


    /**
     * TX SOCKET
     */

    /* Open RAW socket to send on */
    if ((ctx->sockfd = socket(AF_PACKET, SOCK_RAW, htobe16(CSP_ETH_TYPE_CSP))) == -1) {
        perror("socket");
        char exe[1024];
        int count = readlink("/proc/self/exe", exe, sizeof(exe));
        if (count > 0) {
            csp_print("Use command 'sudo setcap cap_net_raw+ep %s'\n", exe);
        }
        return CSP_ERR_INVAL;
    }

    /* Get the index of the interface to send on */
    memset(&ctx->if_idx, 0, sizeof(struct ifreq));
    strncpy(ctx->if_idx.ifr_name, device, IFNAMSIZ-1);
    if (ioctl(ctx->sockfd, SIOCGIFINDEX, &ctx->if_idx) < 0) {
        perror("SIOCGIFINDEX");
        return CSP_ERR_INVAL;
    }

    struct ifreq if_mac;
    /* Get the MAC address of the interface to send on */
    memset(&if_mac, 0, sizeof(struct ifreq));
    strncpy(if_mac.ifr_name, device, IFNAMSIZ-1);
    if (ioctl(ctx->sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
        perror("SIOCGIFHWADDR");
        return CSP_ERR_INVAL;
    }

    memcpy(&ctx->ifdata.if_mac, if_mac.ifr_hwaddr.sa_data, sizeof(ctx->ifdata.if_mac));

    csp_print("INIT %s %s idx %d node %d mac %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", 
        ifname, device, ctx->if_idx.ifr_ifindex, node_id,
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[0],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[1],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[2],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[3],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[4],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[5]);

    /* Allow the socket to be reused - incase connection is closed prematurely */
    int sockopt;
    if (setsockopt(ctx->sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
        perror("setsockopt");
        close(ctx->sockfd);
        return CSP_ERR_INVAL;
    }

    /* Bind to device */
    if (setsockopt(ctx->sockfd, SOL_SOCKET, SO_BINDTODEVICE, device, IFNAMSIZ-1) == -1)	{
        perror("SO_BINDTODEVICE");
        close(ctx->sockfd);
        return CSP_ERR_INVAL;
    }

    /* fill sockaddr_ll struct to prepare binding */
    struct sockaddr_ll my_addr;
    my_addr.sll_family = AF_PACKET;
    my_addr.sll_protocol = htobe16(CSP_ETH_TYPE_CSP);
    my_addr.sll_ifindex = ctx->if_idx.ifr_ifindex;

    /* bind socket  */
    bind(ctx->sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_ll));

    ctx->ifdata.tx_mtu = mtu;

    /* Start server thread */
    static pthread_t server_handle;
    pthread_create(&server_handle, NULL, &csp_eth_rx_loop, ctx);

    /**
     * CSP INTERFACE
     */

    /* Register interface */
    csp_iflist_add(&ctx->ifdata.iface);

	if (return_iface) {
		*return_iface = &ctx->ifdata.iface;
	}

    return CSP_ERR_NONE;}
