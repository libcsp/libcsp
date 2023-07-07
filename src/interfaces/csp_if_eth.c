
#include <csp/interfaces/csp_if_eth.h>
#include <csp/interfaces/csp_if_eth_pbuf.h>

#include <endian.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/csp_interface.h>

bool eth_debug = true;

/* Max number of payload bytes per ETH frame, which is the Ethernet MTU */
#define ETH_FRAME_SIZE_MAX 1500

#define BUF_SIZE    3000

/* Declarations as found in Linux net/ethernet.h and linux/if_ether.h */
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
struct ether_header
{
  uint8_t  ether_dhost[ETH_ALEN];	/* destination eth addr	*/
  uint8_t  ether_shost[ETH_ALEN];	/* source ether addr	*/
  uint16_t ether_type;		        /* packet type ID field	*/
} __attribute__ ((__packed__));


/** ARP : Build list of known (CSP SRC, ETH MAC) 
 * Broadcasting when unknown, then unicasting.
 */

#define ARP_MAX_ENTRIES 10

typedef struct arp_list_entry_s {
    uint16_t csp_addr;
    uint8_t mac_addr[ETH_ALEN];
    struct arp_list_entry_s * next;
} arp_list_entry_t;

static arp_list_entry_t arp_array[ARP_MAX_ENTRIES];
static size_t arp_used = 0;

static arp_list_entry_t * arp_list = 0; 

arp_list_entry_t * arp_alloc() {
    
    if (arp_used >= ARP_MAX_ENTRIES) {
        return 0;
    } 
    return &(arp_array[arp_used++]);

}

void arp_print()
{
    printf("ARP  CSP  MAC\n");
    for (arp_list_entry_t * arp = arp_list; arp; arp = arp->next) {
        printf("     %3u  ", (unsigned)(arp->csp_addr));
        for (int i = 0; i < ETH_ALEN; ++i) {
            printf("%02x ", arp->mac_addr[i]);
        }
    }
    printf("\n");
}

void arp_set_addr(uint16_t csp_addr, uint8_t * mac_addr) 
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
    arp_list_entry_t * new_arp = arp_alloc(sizeof(arp_list_entry_t));

    if (new_arp) {
        new_arp->csp_addr = csp_addr;
        memcpy(new_arp->mac_addr, mac_addr, ETH_ALEN);
        new_arp->next = 0;

        if (last_arp) {
            last_arp->next = new_arp;
        } else {
            arp_list = new_arp;
        }
    }

}

void arp_get_addr(uint16_t csp_addr, uint8_t * mac_addr) 
{
    for (arp_list_entry_t * arp = arp_list; arp ; arp = arp->next) {
        if (arp->csp_addr == csp_addr) {
            memcpy(mac_addr, arp->mac_addr, ETH_ALEN);
            return;
        }
    }
    // Defaults to returning the broadcast address
	memset(mac_addr, 0xff, ETH_ALEN);
}


/* A single list assumes a singledevice */
static csp_packet_t * dma_to_task_list = 0;

void csp_if_eth_dma_rx_callback(void * pbuf, size_t pbuf_size) {

    /* Ethernet header */
    struct ether_header * eh = (struct ether_header *)pbuf;

    /* Filter : ether head (14) + packet length + CSP head */
    if (pbuf_size < sizeof(struct ether_header) + CSP_IF_ETH_PBUF_HEAD_SIZE + 6) {
        printf("pbuf size(%u) < header(%u)\n", (unsigned)pbuf_size, (unsigned)(sizeof(struct ether_header) + CSP_IF_ETH_PBUF_HEAD_SIZE + 6));
        csp_buffer_free_isr(pbuf);
        return;
    }

    /* Filter on CSP protocol id */
    if ((ntohs(eh->ether_type) != ETH_TYPE_CSP)) {
        if (ntohs(eh->ether_type) != 0x0800) {
            /* Exclude IP frames, as these are not errors */
            printf("eth_type(%04x) != %04x\n", (unsigned)ntohs(eh->ether_type), (unsigned)ETH_TYPE_CSP);
        }
        csp_buffer_free_isr(pbuf);
        return;
    }

    /* Pass to task for further processing */
    ((csp_packet_t*)pbuf)->next = dma_to_task_list;
    dma_to_task_list = pbuf;

}

void csp_if_eth_rx_loop(csp_iface_t * iface) {

    csp_packet_t * pbuf_list = 0;

    printf("csp_if_eth_rx_loop '%s'\n", iface->name);

    while (1) {

        /* Get oldest buffer from dma in critical region */

        /* In FreeRTOS the insert call is from ISR that is disabled here */
        #ifdef portENTER_CRITICAL
            taskENTER_CRITICAL();
        #endif

        csp_packet_t * pbuf = dma_to_task_list;
        csp_packet_t * pbuf_prev = 0;
        while (pbuf && pbuf->next) {
            pbuf_prev = pbuf;
            pbuf = pbuf->next;
        }
        if (pbuf_prev) {
            pbuf_prev->next = 0;
        } else {
            dma_to_task_list = 0;
        }

        /* In FreeRTOS the insert call is from ISR that is disabled here */
        #ifdef portEXIT_CRITICAL
            taskEXIT_CRITICAL();
        #endif

        if (!pbuf) {
        	usleep(10000);
            continue;
        }

        /* Cast to byte array */
        uint8_t * recvbuf = (uint8_t*)pbuf;

        /* Ethernet header */
        struct ether_header * eh = (struct ether_header *)recvbuf;

        uint16_t seg_offset = sizeof(struct ether_header);

        uint16_t packet_id = 0;
        uint16_t src_addr = 0;
        uint16_t seg_size = 0;
        uint16_t frame_length = 0;
        seg_offset += csp_if_eth_pbuf_unpack_head(&recvbuf[seg_offset], &packet_id, &src_addr, &seg_size, &frame_length);

        if (seg_size == 0) {
            printf("eth rx seg_size is zero\n");
            csp_buffer_free(pbuf);
            continue;
        }

        if (seg_size > frame_length) {
            printf("eth rx seg_size(%u) > frame_length(%u)\n", (unsigned)seg_size, (unsigned)frame_length);
            csp_buffer_free(pbuf);
            continue;
        }

        if (frame_length == 0) {
            printf("eth rx frame_length is zero\n");
            csp_buffer_free(pbuf);
            continue;
        }

        /* Add packet segment */

        /* Get buffer with same (MAC,SRC) or a new packet if found */
        csp_packet_t * packet = csp_if_eth_pbuf_get(&pbuf_list, csp_if_eth_pbuf_id_as_int32(&recvbuf[sizeof(struct ether_header)]), true);

        if (packet->frame_length == 0) {
            /* First segment */
            packet->frame_length = frame_length;
            packet->rx_count = 0;
        }

        if (frame_length != packet->frame_length) {
            printf("eth rx inconsistent frame_length\n");
            continue;
        }

        if (packet->rx_count + seg_size > packet->frame_length) {
            printf("eth rx data received exceeds frame_length\n");
            continue;
        }

        memcpy(packet->frame_begin + packet->rx_count, &recvbuf[seg_offset], seg_size);
        packet->rx_count += seg_size;
        seg_offset += seg_size;

        // Free segment buffer */
        csp_buffer_free(pbuf);

        /* Send packet when fully received */

        if (packet->rx_count == packet->frame_length) {

            csp_if_eth_pbuf_remove(&pbuf_list, packet);

            if (csp_id_strip(packet) != 0) {
                printf("eth rx packet discarded due to error in ID field\n");
                iface->rx_error++;
                csp_buffer_free(packet);
                continue;
            }

            // Record (CSP SRC,MAC) addresses of source
            arp_set_addr(packet->id.src, eh->ether_shost);

            csp_qfifo_write(packet, iface, NULL);

        }

        if (eth_debug) csp_if_eth_pbuf_list_print(&pbuf_list);

        /* Remove potentially stalled partial packets */

        csp_if_eth_pbuf_list_cleanup(&pbuf_list);

    }

}

int csp_eth_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	/* Loopback */
	if (packet->id.dst == iface->addr) {
		csp_qfifo_write(packet, iface, NULL);
		return CSP_ERR_NONE;
	}

    /* Validate and exclude src zero and stdbuf packets.
       TODO: Should be removed after testing. */
	if ((packet->id.sport == 15) || (packet->id.dport == 15)) {
        return CSP_ERR_NONE;
    }

	csp_eth_interface_data_t * ifdata = iface->interface_data;

    if (!ifdata->init_done) {
        printf("ETH not initialized. Packet dropped.\n");
    	return CSP_ERR_NONE;
    }

    /* Update packet id */
    ifdata->packet_id++;

	static uint8_t sendbuf[BUF_SIZE];

	/* Construct the Ethernet header */
    struct ether_header *eh = (struct ether_header *) sendbuf;

    memcpy(eh->ether_shost, ifdata->mac_addr, ETH_ALEN);
 
    /* Use mac address observed in packet from dst, otherwise broadcast */
    arp_get_addr(packet->id.dst, (uint8_t*)(eh->ether_dhost)); 

	eh->ether_type = ifdata->eth_type;

printf("\n%s:%d TX ETH TYPE %02x %02x  %04x\n", __FILE__, __LINE__, (unsigned)sendbuf[12], (unsigned)sendbuf[13], (unsigned)eh->ether_type);

	csp_id_prepend(packet);

int cdpp = csp_dbg_packet_print;
csp_dbg_packet_print = 2;
csp_print_packet("S %u, D %u, Dp %u, Sp %u, Pr %u, Fl 0x%02X, Sz %" PRIu16 " VIA: %s, %p %p %u\n",
                packet->id.src, packet->id.dst, packet->id.dport,
                packet->id.sport, packet->id.pri, packet->id.flags, packet->length, iface->name, 
                packet, packet->frame_begin, packet->frame_length);
for (size_t i = 0; i < packet->frame_length; ++i) {
    printf(" %02x", *(uint8_t*)(packet->frame_begin + i));
    if (i % 32 == 31) {
        printf("\n");
    }
}
printf("\n");
csp_dbg_packet_print = cdpp;

printf("%s:%d packet %p begin:%p frame_length:%u length:%u\n", __FILE__, __LINE__, packet, packet->frame_begin, (unsigned)packet->frame_length, (unsigned)packet->length);
csp_hex_dump("csp_eth_tx packet", (void*)packet->frame_begin, packet->frame_length);

    uint16_t offset = 0;
    uint16_t seg_offset = 0;
    uint16_t seg_size_max = ifdata->mtu - sizeof(struct ether_header) - CSP_IF_ETH_PBUF_HEAD_SIZE;
    uint16_t seg_size = 0;

    while (offset < packet->frame_length) {

        seg_size = packet->frame_length - offset;
        if (seg_size > seg_size_max) {
            seg_size = seg_size_max;
        }
        
        /* The ethernet header is the same, so reused */
        seg_offset = sizeof(struct ether_header); 

        seg_offset += csp_if_eth_pbuf_pack_head(&sendbuf[seg_offset], ifdata->packet_id, packet->id.src, seg_size, packet->frame_length);

        memcpy(&sendbuf[seg_offset], packet->frame_begin + offset, seg_size);
        seg_offset += seg_size;

        if ((ifdata->tx_func)(iface->driver_data, sendbuf, seg_offset) != CSP_ERR_NONE) {
            iface->tx_error++;
            /* Does not free on return */
            return CSP_ERR_DRIVER;
        }

        offset += seg_size;
    }

    csp_buffer_free(packet);
    return CSP_ERR_NONE;
}

int csp_eth_add_interface(csp_iface_t * iface) {

	if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
		return CSP_ERR_INVAL;
	}

	csp_eth_interface_data_t * ifdata = iface->interface_data;
    if (ifdata) {
	    ifdata->packet_id = 0;
    }



    iface->nexthop = csp_eth_tx;

	return csp_iflist_add(iface);
}
