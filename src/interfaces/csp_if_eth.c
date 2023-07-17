
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

arp_list_entry_t * arp_alloc() {
    
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
        memcpy(new_arp->mac_addr, mac_addr, CSP_ETH_ALEN);
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
            memcpy(mac_addr, arp->mac_addr, CSP_ETH_ALEN);
            return;
        }
    }
    // Defaults to returning the broadcast address
    memset(mac_addr, 0xff, CSP_ETH_ALEN);
}
