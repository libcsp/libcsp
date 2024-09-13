/****************************************************************************
 * **File:** csp/interfaces/csp_if_eth_pbuf.h
 *
 * **Description:** Packet buffer header
 *
 * The first part is an identifier that must uniquely identify the CSP packet
 * to which the segment carried by the pbuf belongs. Each source node sends via
 * same ethernet tx function that must provide the packet identifier like a packet
 * counter that wraps around at 32768. With multiple nodes connected on ethernet
 * the packet identifiers on different nodes may collide, which is why the CSP
 * source address is included in the pbuf identifier. Multiple source addresses
 * causes no problems, as the packet_id dows not depend on the packet content.
 *
 * A segment identifier is considered not required, as the stream of ethernet
 * frames follows a single path, and there are no retransmission, so it can be
 * assumed received in same order as sent. A lost frame causes a stalled pbuf
 * waiting fhr last bytes to arrive. This is discarded a period of
 * CSP_IF_ETH_PBUF_TIMEOUT_MS after latest data was received.
 *
 * There is a minimum ethernet frame size, such that the received size is at
 * leas 60 bytes. The segment size field of the header provides the length of
 * the actual payload.
 *
 * The size of a CSP packet can generally not be determined from a partial CSP
 * frame. The packet_length field providing the packet length is simplest solution.
 ****************************************************************************/
#pragma once

#include <csp/csp.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <csp/interfaces/csp_if_eth.h>

typedef struct {
	uint16_t rx_count;          /* Received bytes */
	uint16_t remain;            /* Remaining packets */
	uint32_t cfpid;             /* Connection CFP identification number */
	uint32_t last_used;         /* Timestamp in ms for last use of buffer */
} csp_eth_pbuf_element_t;

void csp_eth_pbuf_free(csp_eth_interface_data_t * ifdata, csp_packet_t * buffer, int buf_free, int * task_woken);
csp_packet_t * csp_eth_pbuf_new(csp_eth_interface_data_t * ifdata, uint32_t id, uint32_t now, int * task_woken);
csp_packet_t * csp_eth_pbuf_find(csp_eth_interface_data_t * ifdata, uint32_t id, int * task_woken);
void csp_eth_pbuf_cleanup(csp_eth_interface_data_t * ifdata, uint32_t now, int * task_woken);
