#pragma once

#include <stdatomic.h>

#include <csp/csp.h>
#include <csp/arch/csp_queue.h>
#include "csp_semaphore.h"

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

#define CSP_RDP_CLOSED_BY_USERSPACE 0x01
#define CSP_RDP_CLOSED_BY_PROTOCOL  0x02
#define CSP_RDP_CLOSED_BY_TIMEOUT   0x04
#define CSP_RDP_CLOSED_BY_ALL       (CSP_RDP_CLOSED_BY_USERSPACE | CSP_RDP_CLOSED_BY_PROTOCOL | CSP_RDP_CLOSED_BY_TIMEOUT)

/**
 * RDP Connection
 */
typedef struct {
	csp_rdp_state_t state; /**< Connection state */
	uint8_t closed_by;     /**< Tracks 'who' have closed the RDP connection */
	uint16_t snd_nxt;      /**< The sequence number of the next segment that is to be sent */
	uint16_t snd_una;      /**< The sequence number of the oldest unacknowledged segment */
	uint16_t snd_iss;      /**< The initial send sequence number */
	uint16_t rcv_cur;      /**< The sequence number of the last segment received correctly and in sequence */
	uint16_t rcv_irs;      /**< The initial receive sequence number */
	uint16_t rcv_lsa;      /**< The last sequence number acknowledged by the receiver */
	uint32_t window_size;
	uint32_t conn_timeout;
	uint32_t packet_timeout;
	uint32_t delayed_acks;
	uint32_t ack_timeout;
	uint32_t ack_delay_count;
	uint32_t ack_timestamp;
	csp_bin_sem_t tx_wait;

} csp_rdp_t;

/** @brief Connection struct */
struct csp_conn_s {
	atomic_int type;   /* Connection type (CONN_CLIENT or CONN_SERVER) */
	atomic_int state; /* Connection state (CONN_OPEN or CONN_CLOSED) */
	csp_id_t idin;          /* Identifier received */
	csp_id_t idout;         /* Identifier transmitted */
	uint8_t sport_outgoing; /* When used for outgoing, use this sport */

	csp_queue_handle_t rx_queue;        /* Queue for RX packets */
	csp_static_queue_t rx_queue_static; /* Static storage for rx queue */
	char rx_queue_static_data[sizeof(csp_packet_t *) * CSP_CONN_RXQUEUE_LEN];

	void (*callback)(csp_packet_t * packet);

	csp_socket_t * dest_socket; /* incoming connections destination socket */
	uint32_t timestamp;         /* Time the connection was opened */
	uint32_t opts;              /* Connection or socket options */
#if (CSP_USE_RDP)
	csp_rdp_t rdp; /* RDP state */
#endif
};



int csp_conn_enqueue_packet(csp_conn_t * conn, csp_packet_t * packet);
void csp_conn_init(void);
csp_conn_t * csp_conn_allocate(csp_conn_type_t type);

csp_conn_t * csp_conn_find_existing(csp_id_t * id);
csp_conn_t * csp_conn_find_dport(unsigned int dport);

csp_conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout, csp_conn_type_t type);
void csp_conn_check_timeouts(void);
int csp_conn_get_rxq(int prio);
int csp_conn_close(csp_conn_t * conn, uint8_t closed_by);
const csp_conn_t * csp_conn_get_array(size_t * size);  // for test purposes only!
