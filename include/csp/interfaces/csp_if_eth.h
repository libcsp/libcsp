/****************************************************************************
 * **File:** csp/interfaces/csp_if_eth.h
 *
 * **Description:** Ethernet (ETH) interface.
 *
 * Ethernet frames holds up to 1500 bytes, and may be limited by an MTU value
 * that is lower, so in order to transmit CSP packets larger than this,
 * a segmentation protocol is required. The Ethernet standard assumes that this
 * is handled by an upper lat protocol, like bt the IP protocol. This Ethernet
 * driver therefore implements a CSP segmentation protocol that is called EFP.
 * It is similar to CAN CFP.
 *
 * Each packet segment is prepended an EFP header with the following content:
 *
 * - Version:           1 bit
 * - Unused:            2 bit
 * - SegmentId:         5 bit
 * - PacketId:          8 bit
 * - PacketLengthMSB:   8 bit
 * - PacketLengthLSB:   8 bit
 *
 * The Version is zero. EFP discard Ethernet frames with the leading bit being 1.
 * For a potential new protocol to coexist with EFP the first bit in the meader
 * must be 1. The Unused bits must be zero. The SegmentId is a sequential number
 * that starts at zero for each packet and increments by one for each segment.
 * It wraps around to zero at 32, implying that the larges CSP packet size that
 * can be handled is 32 * MTU. The PacketId is a sequential number that is same
 * for all segments of the same packet, and increments by one for every new packet.
 * It wraps around to zero at 256, implying that the protocol can handle up to
 * 256 packets that are processed in parallel. The PacketLengthMSB and
 * PacketLengthLSB holds the packet length most and least significant bits.
 *
 * The protocol assumes that a single segment is sent per Ethernet frame.
 * If smaller than the Ethernet payload length zero-padding is applied.
 * Padding may only occur when transferring the last segment. The protocol allows
 * for segments being received out of order.
 *
 * The protocol differs from CAN CFP by not explicitly storing CSP source and
 * destination addresses in the EFP header. The packets are processed individually,
 * not per CSP connection.
 ****************************************************************************/
#pragma once

#include <csp/csp_interface.h>

/* 0x88B5 : IEEE Std 802 - Local Experimental Ethertype  (RFC 5342) */
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

/**
 * Definition of ethernet header, including reqired MAC addresses
 * Frame data is required to proceed in memory space after this struct
 */
typedef struct csp_eth_header_s
{
	uint8_t  ether_dhost[CSP_ETH_ALEN];	/* destination eth addr	*/
	uint8_t  ether_shost[CSP_ETH_ALEN];	/* source ether addr	*/
	uint16_t ether_type;                /* packet type ID field	*/
	uint16_t packet_id;
	uint16_t src_addr;
	uint16_t seg_size;
	uint16_t packet_length;
	uint8_t frame_begin[];
} __attribute__ ((__packed__)) csp_eth_header_t;

/**
 * Send ETH frame (implemented by driver).
 *
 * Used by csp_eth_tx() to send ETH frames.
 *
 * @param[in] driver_data driver data from #csp_iface_t
 * @param[in] eth_frame CSP Ethernet frame to transmit
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
typedef int (*csp_eth_driver_tx_t)(void * driver_data, csp_eth_header_t * eth_frame);

/**
 * CSP Interface data.
 */
typedef struct {
	csp_iface_t iface;
	bool promisc;
	uint16_t tx_mtu;
	csp_eth_driver_tx_t tx_func;
	csp_eth_header_t * tx_buf;
	csp_packet_t * pbufs;
	uint8_t if_mac[CSP_ETH_ALEN];
} csp_eth_interface_data_t;

/**
 * Pack and byteswapp CSP ethernet header element as required
 */
bool csp_eth_pack_header(csp_eth_header_t * buf,
						 uint16_t packet_id, uint16_t src_addr,
						 uint16_t seg_size, uint16_t packet_length);

/**
 * Unpack CSP ethernet header element
 */
bool csp_eth_unpack_header(csp_eth_header_t * buf,
						   uint32_t * packet_id,
						   uint16_t * seg_size, uint16_t * packet_length);

/**
 * Store MAC address for given CSP node
 */
void csp_eth_arp_set_addr(uint8_t * mac_addr, uint16_t csp_addr);

/**
 * Find MAC address for given CSP node
 */
void csp_eth_arp_get_addr(uint8_t * mac_addr, uint16_t csp_addr);

/**
 * Send CSP packet over CAN (nexthop).
 *
 * This function will split the CSP packet into several fragments and call
 * csp_eth_tx_frame() for sending each fragment.
 *
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_eth_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me);

/**
 * Process received CSP ethernet frame.
 *
 * Called from driver when a single ethernet frame has been received.
 * The function will gather the fragments into a single CSP packet and route
 * it on when complete.
 *
 * @param[in] iface incoming interface.
 * @param[in] eth_frame received ETH data.
 * @param[in] received_len received ETH data length.
 * @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_eth_rx(csp_iface_t * iface, csp_eth_header_t * eth_frame, uint32_t received_len, int * task_woken);
