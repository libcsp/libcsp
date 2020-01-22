/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk) 

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _CSP_CONN_H_
#define _CSP_CONN_H_

#include <csp/csp.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CSP_USE_RDP_FAST_CLOSE
#define CSP_USE_RDP_FAST_CLOSE 0
#endif

/** Connection states */
typedef enum {
	CONN_CLOSED = 0,
	CONN_OPEN = 1,
} csp_conn_state_t;

/** Connection types */
typedef enum {
	CONN_CLIENT = 0,
	CONN_SERVER = 1,
} csp_conn_type_t;

/** RDP Connection states */
typedef enum {
	RDP_CLOSED = 0,
	RDP_SYN_SENT,
	RDP_SYN_RCVD,
	RDP_OPEN,
	RDP_CLOSE_WAIT,
} csp_rdp_state_t;

#define CSP_RDP_CLOSED_BY_USERSPACE  0x01
#define CSP_RDP_CLOSED_BY_PROTOCOL   0x02
#define CSP_RDP_CLOSED_BY_TIMEOUT    0x04
#define CSP_RDP_CLOSED_BY_ALL        (CSP_RDP_CLOSED_BY_USERSPACE | CSP_RDP_CLOSED_BY_PROTOCOL | CSP_RDP_CLOSED_BY_TIMEOUT)

/**
 * RDP Connection
 */
typedef struct {
	csp_rdp_state_t state;		/**< Connection state */
	uint8_t closed_by;		/**< Tracks 'who' have closed the RDP connection */
	uint16_t snd_nxt;		/**< The sequence number of the next segment that is to be sent */
	uint16_t snd_una;		/**< The sequence number of the oldest unacknowledged segment */
	uint16_t snd_iss;		/**< The initial send sequence number */
	uint16_t rcv_cur;		/**< The sequence number of the last segment received correctly and in sequence */
	uint16_t rcv_irs;		/**< The initial receive sequence number */
	uint16_t rcv_lsa;		/**< The last sequence number acknowledged by the receiver */
	uint32_t window_size;
	uint32_t conn_timeout;
	uint32_t packet_timeout;
	uint32_t delayed_acks;
	uint32_t ack_timeout;
	uint32_t ack_delay_count;
	uint32_t ack_timestamp;
	csp_bin_sem_handle_t tx_wait;
	csp_queue_handle_t tx_queue;
	csp_queue_handle_t rx_queue;
} csp_rdp_t;

/** @brief Connection struct */
struct csp_conn_s {
	csp_conn_type_t type;		/* Connection type (CONN_CLIENT or CONN_SERVER) */
	csp_conn_state_t state;		/* Connection state (CONN_OPEN or CONN_CLOSED) */
	csp_id_t idin;			/* Identifier received */
	csp_id_t idout;			/* Identifier transmitted */
#if (CSP_USE_QOS)
	csp_queue_handle_t rx_event;	/* Event queue for RX packets */
#endif
	csp_queue_handle_t rx_queue[CSP_RX_QUEUES]; /* Queue for RX packets */
	csp_queue_handle_t socket;	/* Socket to be "woken" when first packet is ready */
	uint32_t timestamp;		/* Time the connection was opened */
	uint32_t opts;			/* Connection or socket options */
#if (CSP_USE_RDP)
	csp_rdp_t rdp;			/* RDP state */
#endif
};

int csp_conn_enqueue_packet(csp_conn_t * conn, csp_packet_t * packet);
int csp_conn_init(void);
csp_conn_t * csp_conn_allocate(csp_conn_type_t type);
csp_conn_t * csp_conn_find(uint32_t id, uint32_t mask);
csp_conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout);
void csp_conn_check_timeouts(void);
int csp_conn_get_rxq(int prio);
int csp_conn_close(csp_conn_t * conn, uint8_t closed_by);

const csp_conn_t * csp_conn_get_array(size_t * size); // for test purposes only!
void csp_conn_free_resources(void);

#ifdef __cplusplus
}
#endif
#endif
