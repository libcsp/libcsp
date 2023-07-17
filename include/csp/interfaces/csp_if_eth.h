#pragma once

/**
 @file

Ethernet (ETH) interface.

Ethernet frames holds up to 1500 bytes, and may be limited by an MTU value that is lower, so in order to transmit CSP 
packets larger than this, a segmentation protocol is required. The Ethernet standard assumes that this is handled by
an upper lat protocol, like bt the IP protocol.
This Ethernet driver therefore implements a CSP segmentation protocol that is called \b EFP. It is similar to CAN CFP.  

Each packet segment is prepended an \b EFP header with the following content: 

- Version:           1 bit
- Unused:            2 bit
- SegmentId:         5 bit
- PacketId:          8 bit
- PacketLengthMSB:   8 bit
- PacketLengthLSB:   8 bit

The \b Version is zero. EFP discard Ethernet frames with the leading bit being 1. For a potential new protocol to 
coexist with EFP the first bit in the meader must be 1. 
The \b Unused bits must be zero.  
The \b SegmentId is a sequential number that starts at zero for each packet and increments by one for each segment. 
It wraps around to zero at 32, implying that the larges CSP packet size that can be handled is 32 * MTU.
The \b PacketId is a sequential number that is same for all segments of the same packet, and increments by one
for every new packet. It wraps around to zero at 256, implying that the protocol can handle up to 256 packets
that are processed in parallel.
The \b PacketLengthMSB and \b PacketLengthLSB holds the packet length most and least significant bits.

The protocol assumes that a single segment is sent per Ethernet frame. If smaller than the Ethernet payload length
zero-padding is applied. Padding may only occur when transferring the last segment.
The protocol allows for segments being received out of order.

The protocol differs from CAN CFP by not explicitly storing CSP source and destination addresses in the EFP header.
The packets are processed individually, not per CSP connection.  
*/

#include <csp/csp_interface.h>

// 0x88B5 : IEEE Std 802 - Local Experimental Ethertype  (RFC 5342)
#define CSP_ETH_TYPE_CSP 0x88B5

/* Size of buffer that must be greater than the size of an ethernet frame
   carrying a maximum sized (~2k) CSP packet */
#define CSP_ETH_BUF_SIZE    3000

/* Max number of payload bytes per ETH frame, which is the Ethernet MTU */
#define CSP_ETH_FRAME_SIZE_MAX 1500

/** 
 * Declarations same as found in Linux net/ethernet.h and linux/if_ether.h 
 */

#define CSP_ETH_ALEN	6		    /* Octets in one ethernet addr	 */

struct csp_eth_header_s
{
    uint8_t  ether_dhost[CSP_ETH_ALEN];	/* destination eth addr	*/
    uint8_t  ether_shost[CSP_ETH_ALEN];	/* source ether addr	*/
    uint16_t ether_type;                /* packet type ID field	*/
} __attribute__ ((__packed__));
typedef struct csp_eth_header_s csp_eth_header_t;

void arp_set_addr(uint16_t csp_addr, uint8_t * mac_addr);

void arp_get_addr(uint16_t csp_addr, uint8_t * mac_addr);
