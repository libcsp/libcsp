
#include <csp/interfaces/csp_if_eth.h>
#include <csp/interfaces/csp_if_eth_pbuf.h>
#include <csp/arch/csp_time.h>

#include <endian.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/csp_interface.h>


/**
 * Debugging utilities.
 */
bool eth_debug = false;

bool csp_eth_pack_header(csp_eth_header_t * buf, 
                            uint16_t packet_id, uint16_t src_addr,
                            uint16_t seg_size, uint16_t packet_length) {

    if (buf == NULL) return false;

    buf->packet_id = htobe16(packet_id);
    buf->src_addr = htobe16(src_addr);
    buf->seg_size = htobe16(seg_size);
    buf->packet_length = htobe16(packet_length);

    return true;
}

bool csp_if_eth_unpack_header(csp_eth_header_t * buf, 
                              uint32_t * packet_id,
                              uint16_t * seg_size, uint16_t * packet_length) {

    if (packet_id == NULL) return false;
    if (seg_size == NULL) return false;
    if (packet_length == NULL) return false;

    *packet_id = buf->packet_id << 16 | buf->src_addr;
    *seg_size = be16toh(buf->seg_size);
    *packet_length = be16toh(buf->packet_length);

    return true;
}

/**
 * Address resolution (ARP)
 * All received (ETH MAC, CSP src) are recorded and used to map destination address to MAC addresses,
 * used in uni-cast. Until a packet from a CSP address has been received, ETH broadcast is used to this address. 
 */

#define ARP_MAX_ENTRIES 10

typedef struct arp_list_entry_s {
    uint16_t csp_addr;
    uint8_t mac_addr[CSP_ETH_ALEN];
    struct arp_list_entry_s * next;
} arp_list_entry_t;

static arp_list_entry_t arp_array[ARP_MAX_ENTRIES];
static size_t arp_used = 0;

static arp_list_entry_t * arp_list = 0; 

arp_list_entry_t * arp_alloc(void) {
    
    if (arp_used >= ARP_MAX_ENTRIES) {
        return 0;
    } 
    return &(arp_array[arp_used++]);

}

void arp_print()
{
    csp_print("ARP  CSP  MAC\n");
    for (arp_list_entry_t * arp = arp_list; arp; arp = arp->next) {
        csp_print("     %3u  ", (unsigned)(arp->csp_addr));
        for (int i = 0; i < CSP_ETH_ALEN; ++i) {
            csp_print("%02x ", arp->mac_addr[i]);
        }
    }
    csp_print("\n");
}

void csp_eth_arp_set_addr(uint8_t * mac_addr, uint16_t csp_addr)
{
    arp_list_entry_t * last_arp = 0;
    for (arp_list_entry_t * arp = arp_list; arp; arp = arp->next) {
        last_arp = arp;
        if (arp->csp_addr == csp_addr) {
            // Already set
            return;
        }
    }

    // Create and add a new ARP entry
    arp_list_entry_t * new_arp = arp_alloc();

    if (new_arp) {
        new_arp->csp_addr = csp_addr;
        memcpy(new_arp->mac_addr, mac_addr, CSP_ETH_ALEN);
        new_arp->next = 0;

        if (last_arp) {
            last_arp->next = new_arp;
        } else {
            arp_list = new_arp;
        }
    }

}

void csp_eth_arp_get_addr(uint8_t * mac_addr, uint16_t csp_addr)
{
    for (arp_list_entry_t * arp = arp_list; arp ; arp = arp->next) {
        if (arp->csp_addr == csp_addr) {
            memcpy(mac_addr, arp->mac_addr, CSP_ETH_ALEN);
            return;
        }
    }
    // Defaults to returning the broadcast address
    memset(mac_addr, 0xff, CSP_ETH_ALEN);
}

int csp_eth_rx(csp_iface_t * iface, csp_eth_header_t * eth_frame, uint32_t received_len, int * task_woken) {

	csp_eth_interface_data_t * ifdata = iface->interface_data;
    csp_packet_t * pbuf_list = ifdata->pbufs;

    if (eth_debug) csp_hex_dump("rx", (void*)eth_frame, received_len);

    /* Filter on CSP protocol id */
    if ((be16toh(eth_frame->ether_type) != CSP_ETH_TYPE_CSP)) {
        return CSP_ERR_INVAL;
    }

    if (received_len < sizeof(csp_eth_header_t)) {
        return CSP_ERR_INVAL;
    }

    /* Packet ID on RX side is a concatenation of packet ID on TX side and the source address */
    uint32_t packet_id = 0;
    uint16_t seg_size = 0;
    uint16_t frame_length = 0;
    csp_if_eth_unpack_header(eth_frame, &packet_id, &seg_size, &frame_length);

    if (seg_size == 0) {
        csp_print("eth rx seg_size is zero\n");
        return CSP_ERR_INVAL;
    }

    if (seg_size > frame_length) {
        csp_print("eth rx seg_size(%u) > frame_length(%u)\n", (unsigned)seg_size, (unsigned)frame_length);
        return CSP_ERR_INVAL;
    }

    if (sizeof(csp_eth_header_t) + seg_size > received_len) {
        csp_print("eth rx sizeof(csp_eth_frame_t) + seg_size(%u) > received(%u)\n",
            (unsigned)seg_size, (unsigned)received_len);
        return CSP_ERR_INVAL;
    }

    if (frame_length == 0) {
        csp_print("eth rx frame_length is zero\n");
        return CSP_ERR_INVAL;
    }

    /* Add packet segment */
    csp_packet_t * packet = csp_if_eth_pbuf_get(&pbuf_list, packet_id, task_woken);

    if (packet->frame_length == 0) {
        /* First segment */
        packet->frame_length = frame_length;
        packet->rx_count = 0;
    }

    if (frame_length != packet->frame_length) {
        csp_print("eth rx inconsistent frame_length\n");
        return CSP_ERR_INVAL;
    }

    if (packet->rx_count + seg_size > packet->frame_length) {
        csp_print("eth rx data received exceeds frame_length\n");
        return CSP_ERR_INVAL;
    }

    memcpy(packet->frame_begin + packet->rx_count, eth_frame->frame_begin, seg_size);
    packet->rx_count += seg_size;

    /* Send packet when fully received */

    if (packet->rx_count < packet->frame_length) {
        return CSP_ERR_NONE;
    }

    csp_if_eth_pbuf_remove(&pbuf_list, packet);

    if (csp_id_strip(packet) != 0) {
        csp_print("eth rx packet discarded due to error in ID field\n");
        iface->rx_error++;
        (task_woken) ? csp_buffer_free_isr(packet) : csp_buffer_free(packet);
        return CSP_ERR_INVAL;
    }

    /* Record CSP and MAC addresses of source */
    csp_eth_arp_set_addr(eth_frame->ether_shost, packet->id.src);

    if (packet->id.dst != iface->addr && !ifdata->promisc) {
        (task_woken) ? csp_buffer_free_isr(packet) : csp_buffer_free(packet);
        return CSP_ERR_NONE;
    }

    csp_qfifo_write(packet, iface, task_woken);

    if (eth_debug) csp_if_eth_pbuf_list_print(&pbuf_list);

    /* Remove potentially stalled partial packets */
    csp_if_eth_pbuf_list_cleanup(&pbuf_list);

    return CSP_ERR_NONE;
}

int csp_eth_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	csp_eth_interface_data_t * ifdata = iface->interface_data;

    /* Loopback */
    if (packet->id.dst == iface->addr) {
        csp_qfifo_write(packet, iface, NULL);
        return CSP_ERR_NONE;
    }

    static uint16_t packet_id = 0;
    csp_eth_header_t *eth_frame = ifdata->tx_buf;

    csp_eth_arp_get_addr(eth_frame->ether_dhost, packet->id.dst);

    eth_frame->ether_type = htobe16(CSP_ETH_TYPE_CSP);
    memcpy(eth_frame->ether_shost, ifdata->if_mac, CSP_ETH_ALEN);

    csp_id_prepend(packet);

    packet_id++;

    uint16_t offset = 0;
    const uint16_t seg_size_max = ifdata->tx_mtu - sizeof(csp_eth_header_t);

    while (offset < packet->frame_length) {
        uint16_t seg_size = packet->frame_length - offset;
        if (seg_size > seg_size_max) {
            seg_size = seg_size_max;
        }

        csp_eth_pack_header(eth_frame, packet_id, packet->id.src, seg_size, packet->frame_length);

        memcpy(eth_frame->frame_begin, packet->frame_begin + offset, seg_size);

		if ((ifdata->tx_func)(iface->driver_data, eth_frame) != CSP_ERR_NONE) {
			iface->tx_error++;
			/* Does not free on return */
			return CSP_ERR_DRIVER;
        }

        if (eth_debug) csp_hex_dump("tx", eth_frame, sizeof(csp_eth_header_t) + offset + seg_size);

        offset += seg_size;
    }

    csp_buffer_free(packet);
    return CSP_ERR_NONE;
}
