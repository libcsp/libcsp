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

// Global variables assumes a SINGLE ethernet device
static int sockfd = -1;

static struct ifreq if_idx;
static struct ifreq if_mac;

static uint16_t tx_mtu = 0;

extern bool eth_debug;

int csp_if_eth_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

    /* Loopback */
    if (packet->id.dst == iface->addr) {
        csp_qfifo_write(packet, iface, NULL);
        return CSP_ERR_NONE;
    }

    static uint8_t packet_id = 0;
    static uint8_t sendbuf[CSP_ETH_BUF_SIZE];

    /* Construct the Ethernet header */
    csp_eth_header_t *eh = (csp_eth_header_t *) sendbuf;
    uint16_t head_size = sizeof(csp_eth_header_t);

    memcpy(eh->ether_shost, if_mac.ifr_hwaddr.sa_data, CSP_ETH_ALEN);

    arp_get_addr(packet->id.dst, (uint8_t*)(eh->ether_dhost)); 

    eh->ether_type = htons(CSP_ETH_TYPE_CSP);

    if (eth_debug) csp_print("%s:%d TX ETH TYPE %02x %02x  %04x\n", __FILE__, __LINE__, (unsigned)sendbuf[12], (unsigned)sendbuf[13], (unsigned)eh->ether_type);

    /* Destination socket address */
    struct sockaddr_ll socket_address = {};
    socket_address.sll_ifindex = if_idx.ifr_ifindex;
    socket_address.sll_halen = CSP_ETH_ALEN;
    memcpy(socket_address.sll_addr, eh->ether_dhost, CSP_ETH_ALEN);

    csp_id_prepend(packet);

    packet_id++;

    uint16_t offset = 0;
    uint16_t seg_offset = 0;
    uint16_t seg_size_max = tx_mtu - (head_size + CSP_IF_ETH_PBUF_HEAD_SIZE);
    uint16_t seg_size = 0;

    while (offset < packet->frame_length) {

        seg_size = packet->frame_length - offset;
        if (seg_size > seg_size_max) {
            seg_size = seg_size_max;
        }
        
        /* The ethernet header is the same */
        seg_offset = head_size; 

        seg_offset += csp_if_eth_pbuf_pack_head(&sendbuf[seg_offset], packet_id, packet->id.src, seg_size, packet->frame_length);

        memcpy(&sendbuf[seg_offset], packet->frame_begin + offset, seg_size);
        seg_offset += seg_size;

        if (eth_debug) csp_hex_dump("tx", sendbuf, seg_offset);

        if (sendto(sockfd, sendbuf, seg_offset, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0) {
            csp_buffer_free(packet);
            return CSP_ERR_DRIVER;
        }

        offset += seg_size;
    }

    csp_buffer_free(packet);
    return CSP_ERR_NONE;
}

void * csp_if_eth_rx_loop(void * param) {

    static csp_iface_t * iface;
    iface = param;

    csp_packet_t * pbuf_list = 0;

    static uint8_t recvbuf[CSP_ETH_BUF_SIZE];

    /* Ethernet header */
    csp_eth_header_t * eh = (csp_eth_header_t *)recvbuf;
    uint16_t head_size = sizeof(csp_eth_header_t);

    while(1) {

        /* Receive packet segment */ 

        int received_len = recvfrom(sockfd, recvbuf, CSP_ETH_BUF_SIZE, 0, NULL, NULL);

        if (eth_debug) csp_hex_dump("rx", recvbuf, received_len);

        /* Filter : ether head (14) + packet length + CSP head */
        if (received_len < head_size + CSP_IF_ETH_PBUF_HEAD_SIZE + 6) {
            continue;
        }

        /* Filter on CSP protocol id */
        if ((ntohs(eh->ether_type) != CSP_ETH_TYPE_CSP)) {
            continue;
        }
        
        uint16_t seg_offset = head_size;

        uint16_t packet_id = 0;
        uint16_t src_addr = 0;
        uint16_t seg_size = 0;
        uint16_t frame_length = 0;
        seg_offset += csp_if_eth_pbuf_unpack_head(&recvbuf[seg_offset], &packet_id, &src_addr, &seg_size, &frame_length);

        if (seg_size == 0) {
            csp_print("eth rx seg_size is zero\n");
            continue;
        }

        if (seg_size > frame_length) {
            csp_print("eth rx seg_size(%u) > frame_length(%u)\n", (unsigned)seg_size, (unsigned)frame_length);
            continue;
        }

        if (seg_offset + seg_size > received_len) {
            csp_print("eth rx seg_offset(%u) + seg_size(%u) > received(%u)\n",
                (unsigned)seg_offset, (unsigned)seg_size, (unsigned)received_len);
            continue;
        }

        if (frame_length == 0) {
            csp_print("eth rx frame_length is zero\n");
            continue;
        }

        /* Add packet segment */

        csp_packet_t * packet = csp_if_eth_pbuf_get(&pbuf_list, csp_if_eth_pbuf_id_as_int32(&recvbuf[head_size]), true);

        if (packet->frame_length == 0) {
            /* First segment */
            packet->frame_length = frame_length;
            packet->rx_count = 0;
        }

        if (frame_length != packet->frame_length) {
            csp_print("eth rx inconsistent frame_length\n");
            continue;
        }

        if (packet->rx_count + seg_size > packet->frame_length) {
            csp_print("eth rx data received exceeds frame_length\n");
            continue;
        }

        memcpy(packet->frame_begin + packet->rx_count, &recvbuf[seg_offset], seg_size);
        packet->rx_count += seg_size;
        seg_offset += seg_size;

        /* Send packet when fully received */

        if (packet->rx_count == packet->frame_length) {

            csp_if_eth_pbuf_remove(&pbuf_list, packet);

            if (csp_id_strip(packet) != 0) {
                csp_print("eth rx packet discarded due to error in ID field\n");
                iface->rx_error++;
                csp_buffer_free(packet);
                continue;
            }

            // Record (CSP,MAC) addresses of source
            arp_set_addr(packet->id.src, eh->ether_shost);

            csp_qfifo_write(packet, iface, NULL);

        }

        if (eth_debug) csp_if_eth_pbuf_list_print(&pbuf_list);

        /* Remove potentially stalled partial packets */

        csp_if_eth_pbuf_list_cleanup(&pbuf_list);
    }

    return NULL;
}

void csp_if_eth_init(csp_iface_t * iface, const char * device, const char * ifname, int mtu, bool promisc) {

    /* Ether header 14 byte, seg header 4 byte, arbitrarily min 22 bytes for data */
    if (mtu < 40) {
        csp_print("csp_if_eth_init: mtu < 40\n");
        return;
    }


    /**
     * TX SOCKET
     */

    /* Open RAW socket to send on */
    uint16_t protocol = promisc ? ETH_P_ALL : CSP_ETH_TYPE_CSP;
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(protocol))) == -1) {
        perror("socket");
        return;
    }

    /* Get the index of the interface to send on */
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, device, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
        perror("SIOCGIFINDEX");
        return;
    }

    /* Get the MAC address of the interface to send on */
    memset(&if_mac, 0, sizeof(struct ifreq));
    strncpy(if_mac.ifr_name, device, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
        perror("SIOCGIFHWADDR");
        return;
    }

    csp_print("%s %s idx %d mac %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx Promisc:%u\n", ifname, device, if_idx.ifr_ifindex, 
        ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0],
        ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1],
        ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2],
        ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3],
        ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4],
        ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5],
        (unsigned)promisc);

    /* Allow the socket to be reused - incase connection is closed prematurely */
    int sockopt;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
        perror("setsockopt");
        close(sockfd);
        return;
    }

    /* Bind to device */
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, device, IFNAMSIZ-1) == -1)	{
        perror("SO_BINDTODEVICE");
        close(sockfd);
        return;
    }

    /* fill sockaddr_ll struct to prepare binding */
    struct sockaddr_ll my_addr;
    my_addr.sll_family = AF_PACKET;
    my_addr.sll_protocol = htons(CSP_ETH_TYPE_CSP);
    my_addr.sll_ifindex = if_idx.ifr_ifindex;

    /* bind socket  */
    bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_ll));

    tx_mtu = mtu;

    /* Start server thread */
    static pthread_t server_handle;
    pthread_create(&server_handle, NULL, &csp_if_eth_rx_loop, iface);

    /**
     * CSP INTERFACE
     */

    /* Register interface */
    iface->name = strdup(ifname),
    iface->nexthop = &csp_if_eth_tx,
    csp_iflist_add(iface);

}
