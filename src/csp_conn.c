

#include "csp_autoconfig.h"

#include "csp_conn.h"

#include <stdlib.h>
#include <stdio.h>

#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/arch/csp_time.h>
#include <csp/csp_id.h>
#include "transport/csp_transport.h"

/* Connection pool */
static csp_conn_t arr_conn[CSP_CONN_MAX] __attribute__((section(".noinit")));

/* Connection pool lock */
static csp_bin_sem_handle_t conn_lock __attribute__((section(".noinit")));
static csp_bin_sem_t conn_lock_buf __attribute__((section(".noinit")));

/* Last used 'source' port */
static uint8_t sport;

/* Source port lock */
static csp_bin_sem_handle_t sport_lock __attribute__((section(".noinit")));
static csp_bin_sem_t sport_lock_buf __attribute__((section(".noinit")));

void csp_conn_check_timeouts(void) {
#if (CSP_USE_RDP)
	for (int i = 0; i < CSP_CONN_MAX; i++) {
		if (arr_conn[i].state == CONN_OPEN) {
			if (arr_conn[i].idin.flags & CSP_FRDP) {
				csp_rdp_check_timeouts(&arr_conn[i]);
			}
		}
	}
#endif
}

int csp_conn_enqueue_packet(csp_conn_t * conn, csp_packet_t * packet) {

	if (!conn)
		return CSP_ERR_INVAL;

	if (csp_queue_enqueue(conn->rx_queue, &packet, 0) != CSP_QUEUE_OK) {
		csp_log_error("RX queue %p full with %u items", conn->rx_queue, csp_queue_size(conn->rx_queue));
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;
}

void csp_conn_init(void) {

	csp_bin_sem_create_static(&conn_lock, &conn_lock_buf);
	csp_bin_sem_create_static(&sport_lock, &sport_lock_buf);

	/* Initialize source port */
	unsigned int seed = csp_get_ms();
	sport = (rand_r(&seed) % (csp_id_get_max_port() - CSP_PORT_MAX_BIND)) + (CSP_PORT_MAX_BIND + 1);

	for (int i = 0; i < CSP_CONN_MAX; i++) {
		csp_conn_t * conn = &arr_conn[i];

		conn->state = CONN_CLOSED;
		conn->rx_queue = csp_queue_create_static(CSP_CONN_RXQUEUE_LEN, sizeof(csp_packet_t *), conn->rx_queue_static_data, &conn->rx_queue_static);

#if (CSP_USE_RDP)
		csp_rdp_init(conn);
#endif
	}
}

csp_conn_t * csp_conn_find_dport(unsigned int dport) {

	for (int i = 0; i < CSP_CONN_MAX; i++) {
		csp_conn_t * conn = &arr_conn[i];

		/* Connection must match dport */
		if (conn->idin.dport != dport)
			continue;

		/* Connection must be open */
		if (conn->state != CONN_OPEN)
			continue;

		/* Connection must be client */
		if (conn->type != CONN_CLIENT)
			continue;

		/* All conditions found! */
		return conn;
	}

	return NULL;
}

csp_conn_t * csp_conn_find_existing(csp_id_t * id) {

	for (int i = 0; i < CSP_CONN_MAX; i++) {
		csp_conn_t * conn = &arr_conn[i];

		/**
		 * This search looks verbose, Instead of a big if statement, it is written out as
		 * conditions. This has been done for clarity. The least likely check is put first
		 * for runtime speed improvement.
		 * Also this is written using explicit fields, not bitmasks, in order to improve
		 * portability and dual use between different header formats.
		 */

		/* Connection must match dport */
		if (conn->idin.dport != id->dport)
			continue;

		/* Connection must match sport */
		if (conn->idin.sport != id->sport)
			continue;

		/* Connection must match destination */
		if (conn->idin.dst != id->dst)
			continue;

		/* Connection must match source */
		if (conn->idin.src != id->src)
			continue;

		/* Connection must be open */
		if (conn->state != CONN_OPEN)
			continue;

		/* Connection must be client */
		if (conn->type != CONN_CLIENT)
			continue;

		/* All conditions found! */
		return conn;
	}

	return NULL;
}

static int csp_conn_flush_rx_queue(csp_conn_t * conn) {

	csp_packet_t * packet;

	/* Flush packet queues */
	while (csp_queue_dequeue(conn->rx_queue, &packet, 0) == CSP_QUEUE_OK) {
		if (packet != NULL) {
			csp_buffer_free(packet);
		}
	}

	return CSP_ERR_NONE;
}

csp_conn_t * csp_conn_allocate(csp_conn_type_t type) {

	static uint8_t csp_conn_last_given = 0;

	if (csp_bin_sem_wait(&conn_lock, CSP_MAX_TIMEOUT) != CSP_SEMAPHORE_OK) {
		csp_log_error("Failed to lock conn array");
		return NULL;
	}

	/* Search for free connection */
	csp_conn_t * conn = NULL;
	int i = csp_conn_last_given;
	for (int j = 0; j < CSP_CONN_MAX; j++) {
		i = (i + 1) % CSP_CONN_MAX;
		conn = &arr_conn[i];
		if (conn->state == CONN_CLOSED) {
			break;
		}
	}

	if (conn && (conn->state == CONN_CLOSED)) {
		conn->timestamp = 0;
		conn->type = type;
		conn->state = CONN_OPEN;
		csp_conn_last_given = i;
	} else {
		// no free connections
		conn = NULL;
	}

	csp_bin_sem_post(&conn_lock);

	if (conn == NULL) {
		csp_log_error("No free connections, max %u", CSP_CONN_MAX);
	}

	return conn;
}

csp_conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout) {

	/* Allocate connection structure */
	csp_conn_t * conn = csp_conn_allocate(CONN_CLIENT);

	if (conn) {
		/* No lock is needed here, because nobody else *
		 * has a reference to this connection yet.     */
		csp_id_copy(&conn->idin, &idin);
		csp_id_copy(&conn->idout, &idout);

		conn->timestamp = csp_get_ms();

		/* Ensure connection queue is empty */
		csp_conn_flush_rx_queue(conn);
	}

	return conn;
}

int csp_close(csp_conn_t * conn) {
	return csp_conn_close(conn, CSP_RDP_CLOSED_BY_USERSPACE);
}

int csp_conn_close(csp_conn_t * conn, uint8_t closed_by) {

	if (conn == NULL) {
		return CSP_ERR_NONE;
	}

	if (conn->state == CONN_CLOSED) {
		csp_log_protocol("Conn already closed");
		return CSP_ERR_NONE;
	}

#if (CSP_USE_RDP)
	/* Ensure RDP knows this connection is closing */
	if ((conn->idin.flags & CSP_FRDP) || (conn->idout.flags & CSP_FRDP)) {
		if (csp_rdp_close(conn, closed_by) == CSP_ERR_AGAIN) {
			return CSP_ERR_NONE;
		}
	}
#endif

	/* Lock connection array while closing connection */
	if (csp_bin_sem_wait(&conn_lock, CSP_MAX_TIMEOUT) != CSP_SEMAPHORE_OK) {
		csp_log_error("Failed to lock conn array");
		return CSP_ERR_TIMEDOUT;
	}

	/* Set to closed */
	conn->state = CONN_CLOSED;

	/* Ensure connection queue is empty */
	csp_conn_flush_rx_queue(conn);

	/* Reset RDP state */
#if (CSP_USE_RDP)
	if (conn->idin.flags & CSP_FRDP) {
		csp_rdp_flush_all(conn);
	}
#endif

	/* Unlock connection array */
	csp_bin_sem_post(&conn_lock);

	return CSP_ERR_NONE;
}

csp_conn_t * csp_connect(uint8_t prio, uint16_t dest, uint8_t dport, uint32_t timeout, uint32_t opts) {

	/* Force options on all connections */
	opts |= csp_conf.conn_dfl_so;

	/* Generate identifier */
	csp_id_t incoming_id, outgoing_id;
	incoming_id.pri = prio;
	incoming_id.dst = csp_conf.address;
	incoming_id.src = dest;
	incoming_id.sport = dport;
	incoming_id.flags = 0;
	outgoing_id.pri = prio;
	outgoing_id.dst = dest;
	outgoing_id.src = csp_conf.address;
	outgoing_id.dport = dport;
	outgoing_id.flags = 0;

	/* Set connection options */
	if (opts & CSP_O_NOCRC32) {
		opts &= ~CSP_O_CRC32;
	}

	if (opts & CSP_O_RDP) {
#if (CSP_USE_RDP)
		incoming_id.flags |= CSP_FRDP;
		outgoing_id.flags |= CSP_FRDP;
#else
		csp_log_error("No RDP support");
		return NULL;
#endif
	}

	if (opts & CSP_O_HMAC) {
#if (CSP_USE_HMAC)
		outgoing_id.flags |= CSP_FHMAC;
		incoming_id.flags |= CSP_FHMAC;
#else
		csp_log_error("No HMAC support");
		return NULL;
#endif
	}

	if (opts & CSP_O_XTEA) {
#if (CSP_USE_XTEA)
		outgoing_id.flags |= CSP_FXTEA;
		incoming_id.flags |= CSP_FXTEA;
#else
		csp_log_error("No XTEA support");
		return NULL;
#endif
	}

	if (opts & CSP_O_CRC32) {
		outgoing_id.flags |= CSP_FCRC32;
		incoming_id.flags |= CSP_FCRC32;
	}

	/* Find an unused ephemeral port */
	csp_conn_t * conn = NULL;

	/* Wait for sport lock - note that csp_conn_new(..) is called inside the lock! */
	if (csp_bin_sem_wait(&sport_lock, CSP_MAX_TIMEOUT) != CSP_SEMAPHORE_OK) {
		return NULL;
	}

	/* Loop through available source ports */
	const uint8_t start = sport;
	while (++sport != start) {
		if (sport > csp_id_get_max_port())
			sport = CSP_PORT_MAX_BIND + 1;

		/* Search for ephemeral outgoing port */
		if (csp_conn_find_dport(sport) == NULL) {

			/* We found an unused ephemeral port
			 * allocate connection while locked to mark port in use */

			outgoing_id.sport = sport;
			incoming_id.dport = sport;

			conn = csp_conn_new(incoming_id, outgoing_id);
			break;
		}
	}

	/* Post sport lock */
	csp_bin_sem_post(&sport_lock);

	if (conn == NULL) {
		return NULL;
	}

	/* Set connection options */
	conn->opts = opts;

#if (CSP_USE_RDP)
	/* Call Transport Layer connect */
	if (outgoing_id.flags & CSP_FRDP) {
		/* If the transport layer has failed to connect
		 * deallocate connection structure again and return NULL */
		if (csp_rdp_connect(conn) != CSP_ERR_NONE) {
			csp_close(conn);
			return NULL;
		}
	}
#endif

	/* We have a successful connection */
	return conn;
}

int csp_conn_dport(csp_conn_t * conn) {

	return conn->idin.dport;
}

int csp_conn_sport(csp_conn_t * conn) {

	return conn->idin.sport;
}

int csp_conn_dst(csp_conn_t * conn) {

	return conn->idin.dst;
}

int csp_conn_src(csp_conn_t * conn) {

	return conn->idin.src;
}

int csp_conn_flags(csp_conn_t * conn) {

	return conn->idin.flags;
}

#if (CSP_DEBUG)
void csp_conn_print_table(void) {

	for (unsigned int i = 0; i < CSP_CONN_MAX; i++) {
		csp_conn_t * conn = &arr_conn[i];
		printf("[%02u %p] S:%u, %u -> %u, %u -> %u\r\n",
			   i, conn, conn->state, conn->idin.src, conn->idin.dst,
			   conn->idin.dport, conn->idin.sport);
#if (CSP_USE_RDP)
		if (conn->idin.flags & CSP_FRDP) {
			csp_rdp_conn_print(conn);
		}
#endif
	}
}

int csp_conn_print_table_str(char * str_buf, int str_size) {

	/* Display up to 10 connections */
	unsigned int start = (CSP_CONN_MAX > 10) ? (CSP_CONN_MAX - 10) : 0;

	for (unsigned int i = start; i < CSP_CONN_MAX; i++) {
		csp_conn_t * conn = &arr_conn[i];
		char buf[100];
		snprintf(buf, sizeof(buf), "[%02u %p] S:%u, %u -> %u, %u -> %u\n",
				 i, conn, conn->state, conn->idin.src, conn->idin.dst,
				 conn->idin.dport, conn->idin.sport);

		strncat(str_buf, buf, str_size);
		if ((str_size -= strlen(buf)) <= 0) {
			break;
		}
	}

	return CSP_ERR_NONE;
}
#endif

const csp_conn_t * csp_conn_get_array(size_t * size) {
	*size = CSP_CONN_MAX;
	return arr_conn;
}
