#pragma once

#include <csp/csp.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/** Packet buffer header
 * The first part is an identifier that must uniquely identify the CSP packet to which the 
 * segment carried by the pbuf belongs.
 * Each source node sends via same ethernet tx function that must provide the packet identifier
 * like a packet counter that wraps around at 32768. 
 * With multiple nodes connected on ethernet the packet identifiers on different nodes may collide,
 * which is why the CSP source address is included in the pbuf identifier. Multiple source addresses
 * causes no problems, as the packet_id dows not depend on the packet content.
 * 
 * A segment identifier is considered not required, as the stream of ethernet frames follows a single 
 * path, and there are no retransmission, so it can be assumed received in same order as sent.
 * A lost frame causes a stalled pbuf waiting fhr last bytes to arrive. This is discarded a period
 * of CSP_IF_ETH_PBUF_TIMEOUT_MS after latest data was received.
 * 
 * There is a minimum ethernet frame size, such that the received size is at leas 60 bytes. 
 * The segment size field of the header provides the length of the actual payload.
 * 
 * The size of a CSP packet can generally not be determined from a partial CSP frame.
 * The packet_length field providing the packet length is simplest solution.
*/

#define CSP_IF_ETH_PBUF_HEAD_SIZE 8
#define CSP_IF_ETH_PBUF_TIMEOUT_MS 2000

uint16_t csp_if_eth_pbuf_pack_head(uint8_t * buf, 
                                   uint16_t packet_id, uint16_t src_addr,
                                   uint16_t seg_size, uint16_t packet_length);

uint16_t csp_if_eth_pbuf_unpack_head(uint8_t * buf, 
                                     uint16_t * packet_id, uint16_t * src_addr,
                                     uint16_t * seg_size, uint16_t * packet_length);

uint32_t csp_if_eth_pbuf_id_as_int32(uint8_t * buf);

/** Packet list operations */

csp_packet_t * csp_if_eth_pbuf_find(csp_packet_t ** plist, uint32_t pbuf_id);

void csp_if_eth_pbuf_insert(csp_packet_t ** plist, csp_packet_t * packet);

csp_packet_t * csp_if_eth_pbuf_get(csp_packet_t ** plist, uint32_t pbuf_id, bool isr);

void csp_if_eth_pbuf_remove(csp_packet_t ** plist, csp_packet_t * packet);

void csp_if_eth_pbuf_list_cleanup(csp_packet_t ** plist);

void csp_if_eth_pbuf_print(const char * descr, csp_packet_t * packet);

void csp_if_eth_pbuf_list_print(csp_packet_t ** plist);


