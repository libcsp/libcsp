#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <csp/csp_id.h>
#include <csp/arch/csp_time.h>
#include <csp/interfaces/csp_if_eth_pbuf.h>

/** Packet buffer list operations */

csp_packet_t * csp_if_eth_pbuf_find(csp_packet_t ** plist, uint32_t pbuf_id) {

    csp_packet_t * packet = *plist;
    while(packet) {
        if (packet->cfpid == pbuf_id) {
            return packet;
        }
        packet = packet->next;
    }    
    return packet;

}

void csp_if_eth_pbuf_insert(csp_packet_t ** plist, csp_packet_t * packet) {

    if (*plist) {
        packet->next = *plist;
    }
    *plist = packet;

}

csp_packet_t * csp_if_eth_pbuf_get(csp_packet_t ** plist, uint32_t pbuf_id, int * task_woken) {

    csp_packet_t * packet = csp_if_eth_pbuf_find(plist, pbuf_id);

    if (packet) {
    	packet->last_used = csp_get_ms();
        return packet;
    }

    if (!packet) {
        packet = (task_woken) ? csp_buffer_get_isr(0) : csp_buffer_get(0);
    }

    if (!packet) {
        /* No free packet */
        return NULL;
    }

	csp_id_setup_rx(packet);

    /* Existing cfpid and rx_count fields are used */ 
    packet->cfpid = pbuf_id;
    packet->rx_count = 0;
	packet->last_used = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

    packet->next = 0;
    csp_if_eth_pbuf_insert(plist, packet);

    return packet;

}

void csp_if_eth_pbuf_remove(csp_packet_t ** plist, csp_packet_t * packet) {

    csp_packet_t * prev = 0;
    csp_packet_t * p = *plist;
    while(p && (p != packet)) {
        prev = p;
        p = p->next;
    }   

    if (p) {
        if (prev) {
            prev->next = p->next;
        } else {
            *plist = p->next;
        }
    }

}

void csp_if_eth_pbuf_list_cleanup(csp_packet_t ** plist) {

    /* Free stalled packets, like for which a segment has been lost */
    uint32_t now = csp_get_ms();
    csp_packet_t * packet = *plist;
    while(packet) {
		if (now > packet->last_used + CSP_IF_ETH_PBUF_TIMEOUT_MS) {
            csp_if_eth_pbuf_print("timeout ", packet);
            csp_if_eth_pbuf_remove(plist, packet);
            csp_buffer_free(packet);
        }
        packet = packet->next;
    }

}

void csp_if_eth_pbuf_print(const char * descr, csp_packet_t * packet) {

    if (packet) {
        printf("%s %p id:%u Age:%lu,%lu,%lu flen:%u\n",
            descr, packet, 
            (unsigned)packet->cfpid, 
            (unsigned long)csp_get_ms(), (unsigned long)packet->last_used, (unsigned long)(csp_get_ms() - packet->last_used), 
            (unsigned)packet->frame_length);
    } else {
        printf("Packet is null\n");
    }

}

void csp_if_eth_pbuf_list_print(csp_packet_t ** plist) {

    csp_packet_t * packet = *plist;
    while(packet) {
        csp_if_eth_pbuf_print("list ", packet);
        packet = packet->next;
    }

}

