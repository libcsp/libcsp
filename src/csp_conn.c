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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_error.h>

#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_time.h>

#include "csp_conn.h"
#include "transport/csp_transport.h"

/* Static connection pool */
static csp_conn_t arr_conn[CSP_CONN_MAX];

/* Connection pool lock */
static csp_bin_sem_handle_t conn_lock;

/* Source port */
static uint8_t sport;

/* Source port lock */
static csp_bin_sem_handle_t sport_lock;

void csp_conn_check_timeouts(void) {
#ifdef CSP_USE_RDP
	int i;
	for (i = 0; i < CSP_CONN_MAX; i++)
		if (arr_conn[i].state == CONN_OPEN)
			if (arr_conn[i].idin.flags & CSP_FRDP)
				csp_rdp_check_timeouts(&arr_conn[i]);
#endif
}

int csp_conn_get_rxq(int prio) {

#ifdef CSP_USE_QOS
	return prio;
#else
	return 0;
#endif

}

int csp_conn_lock(csp_conn_t * conn, uint32_t timeout) {

	if (csp_mutex_lock(&conn->lock, timeout) != CSP_MUTEX_OK)
		return CSP_ERR_TIMEDOUT;

	return CSP_ERR_NONE;

}

int csp_conn_unlock(csp_conn_t * conn) {

	csp_mutex_unlock(&conn->lock);

	return CSP_ERR_NONE;

}

int csp_conn_enqueue_packet(csp_conn_t * conn, csp_packet_t * packet) {

	if (!conn)
		return CSP_ERR_INVAL;

	int rxq;
	if (packet != NULL) {
		rxq = csp_conn_get_rxq(packet->id.pri);
	} else {
		rxq = CSP_RX_QUEUES - 1;
	}

	if (csp_queue_enqueue(conn->rx_queue[rxq], &packet, 0) != CSP_QUEUE_OK) {
		csp_log_error("RX queue %p full with %u items", conn->rx_queue[rxq], csp_queue_size(conn->rx_queue[rxq]));
		return CSP_ERR_NOMEM;
	}

#ifdef CSP_USE_QOS
	int event = 0;
	if (csp_queue_enqueue(conn->rx_event, &event, 0) != CSP_QUEUE_OK) {
		csp_log_error("QOS event queue full");
		return CSP_ERR_NOMEM;
	}
#endif

	return CSP_ERR_NONE;
}

int csp_conn_init(void) {

	/* Initialize source port */
	srand(csp_get_ms());
	sport = (rand() % (CSP_ID_PORT_MAX - CSP_MAX_BIND_PORT)) + (CSP_MAX_BIND_PORT + 1);

	if (csp_bin_sem_create(&sport_lock) != CSP_SEMAPHORE_OK) {
		csp_log_error("No more memory for sport semaphore");
		return CSP_ERR_NOMEM;
	}

	int i, prio;
	for (i = 0; i < CSP_CONN_MAX; i++) {
		for (prio = 0; prio < CSP_RX_QUEUES; prio++)
			arr_conn[i].rx_queue[prio] = csp_queue_create(CSP_RX_QUEUE_LENGTH, sizeof(csp_packet_t *));

#ifdef CSP_USE_QOS
		arr_conn[i].rx_event = csp_queue_create(CSP_CONN_QUEUE_LENGTH, sizeof(int));
#endif
		arr_conn[i].state = CONN_CLOSED;

		if (csp_mutex_create(&arr_conn[i].lock) != CSP_MUTEX_OK) {
			csp_log_error("Failed to create connection lock");
			return CSP_ERR_NOMEM;
		}

#ifdef CSP_USE_RDP
		if (csp_rdp_allocate(&arr_conn[i]) != CSP_ERR_NONE) {
			csp_log_error("Failed to create queues for RDP in csp_conn_init");
			return CSP_ERR_NOMEM;
		}
#endif
	}

	if (csp_bin_sem_create(&conn_lock) != CSP_SEMAPHORE_OK) {
		csp_log_error("No more memory for conn semaphore");
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;

}

csp_conn_t * csp_conn_find(uint32_t id, uint32_t mask) {

	/* Search for matching connection */
	int i;
	csp_conn_t * conn;

	for (i = 0; i < CSP_CONN_MAX; i++) {
		conn = &arr_conn[i];
		if ((conn->state != CONN_CLOSED) && (conn->type == CONN_CLIENT) && (conn->idin.ext & mask) == (id & mask))
			return conn;
	}
	
	return NULL;

}

static int csp_conn_flush_rx_queue(csp_conn_t * conn) {

	csp_packet_t * packet;

	int prio;

	/* Flush packet queues */
	for (prio = 0; prio < CSP_RX_QUEUES; prio++) {
		while (csp_queue_dequeue(conn->rx_queue[prio], &packet, 0) == CSP_QUEUE_OK)
			if (packet != NULL)
				csp_buffer_free(packet);
	}

	/* Flush event queue */
#ifdef CSP_USE_QOS
	int event;
	while (csp_queue_dequeue(conn->rx_event, &event, 0) == CSP_QUEUE_OK);
#endif

	return CSP_ERR_NONE;

}

csp_conn_t * csp_conn_allocate(csp_conn_type_t type) {

	int i, j;
	static uint8_t csp_conn_last_given = 0;
	csp_conn_t * conn;

	if (csp_bin_sem_wait(&conn_lock, 100) != CSP_SEMAPHORE_OK) {
		csp_log_error("Failed to lock conn array");
		return NULL;
	}

	/* Search for free connection */
	i = csp_conn_last_given;
	i = (i + 1) % CSP_CONN_MAX;

	for (j = 0; j < CSP_CONN_MAX; j++) {
		conn = &arr_conn[i];
		if (conn->state == CONN_CLOSED)
			break;
		i = (i + 1) % CSP_CONN_MAX;
	}

	if (conn->state == CONN_OPEN) {
		csp_log_error("No more free connections");
		csp_bin_sem_post(&conn_lock);
		return NULL;
	}

	conn->state = CONN_OPEN;
	conn->socket = NULL;
	conn->type = type;
	csp_conn_last_given = i;
	csp_bin_sem_post(&conn_lock);

	return conn;

}

csp_conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout) {

	/* Allocate connection structure */
	csp_conn_t * conn = csp_conn_allocate(CONN_CLIENT);

	if (conn) {
		/* No lock is needed here, because nobody else *
		 * has a reference to this connection yet.     */
		conn->idin.ext = idin.ext;
		conn->idout.ext = idout.ext;
		conn->timestamp = csp_get_ms();

		/* Ensure connection queue is empty */
		csp_conn_flush_rx_queue(conn);
	}

	return conn;

}

int csp_close(csp_conn_t * conn) {

	if (conn == NULL) {
		csp_log_error("NULL Pointer given to csp_close");
		return CSP_ERR_INVAL;
	}

	if (conn->state == CONN_CLOSED) {
		csp_log_protocol("Conn already closed");
		return CSP_ERR_NONE;
	}

#ifdef CSP_USE_RDP
	/* Ensure RDP knows this connection is closing */
	if (conn->idin.flags & CSP_FRDP || conn->idout.flags & CSP_FRDP)
		if (csp_rdp_close(conn) == CSP_ERR_AGAIN)
			return CSP_ERR_NONE;
#endif

	/* Lock connection array while closing connection */
	if (csp_bin_sem_wait(&conn_lock, 100) != CSP_SEMAPHORE_OK) {
		csp_log_error("Failed to lock conn array");
		return CSP_ERR_TIMEDOUT;
	}

	/* Set to closed */
	conn->state = CONN_CLOSED;

	/* Ensure connection queue is empty */
	csp_conn_flush_rx_queue(conn);

	/* Reset RDP state */
#ifdef CSP_USE_RDP
	if (conn->idin.flags & CSP_FRDP)
		csp_rdp_flush_all(conn);
#endif

	/* Unlock connection array */
	csp_bin_sem_post(&conn_lock);

	return CSP_ERR_NONE;
}

csp_conn_t * csp_connect(uint8_t prio, uint8_t dest, uint8_t dport, uint32_t timeout, uint32_t opts) {

	/* Force options on all connections */
	opts |= CSP_CONNECTION_SO;

	/* Generate identifier */
	csp_id_t incoming_id, outgoing_id;
	incoming_id.pri = prio;
	incoming_id.dst = csp_get_address();
	incoming_id.src = dest;
	incoming_id.sport = dport;
	incoming_id.flags = 0;
	outgoing_id.pri = prio;
	outgoing_id.dst = dest;
	outgoing_id.src = csp_get_address();
	outgoing_id.dport = dport;
	outgoing_id.flags = 0;

	/* Set connection options */
	if (opts & CSP_O_RDP) {
#ifdef CSP_USE_RDP
		incoming_id.flags |= CSP_FRDP;
		outgoing_id.flags |= CSP_FRDP;
#else
		csp_log_error("Attempt to create RDP connection, but CSP was compiled without RDP support");
		return NULL;
#endif
	}

	if (opts & CSP_O_HMAC) {
#ifdef CSP_USE_HMAC
		outgoing_id.flags |= CSP_FHMAC;
		incoming_id.flags |= CSP_FHMAC;
#else
		csp_log_error("Attempt to create HMAC authenticated connection, but CSP was compiled without HMAC support");
		return NULL;
#endif
	}

	if (opts & CSP_O_XTEA) {
#ifdef CSP_USE_XTEA
		outgoing_id.flags |= CSP_FXTEA;
		incoming_id.flags |= CSP_FXTEA;
#else
		csp_log_error("Attempt to create XTEA encrypted connection, but CSP was compiled without XTEA support");
		return NULL;
#endif
	}

	if (opts & CSP_O_CRC32) {
#ifdef CSP_USE_CRC32
		outgoing_id.flags |= CSP_FCRC32;
		incoming_id.flags |= CSP_FCRC32;
#else
		csp_log_error("Attempt to create CRC32 validated connection, but CSP was compiled without CRC32 support");
		return NULL;
#endif
	}
	
	/* Find an unused ephemeral port */
	csp_conn_t * conn;

	/* Wait for sport lock */
	if (csp_bin_sem_wait(&sport_lock, 1000) != CSP_SEMAPHORE_OK)
		return NULL;

	uint8_t start = sport;
	while (++sport != start) {
		if (sport > CSP_ID_PORT_MAX)
			sport = CSP_MAX_BIND_PORT + 1;

		outgoing_id.sport = sport;
		incoming_id.dport = sport;
		
		/* Match on destination port of _incoming_ identifier */
		conn = csp_conn_find(incoming_id.ext, CSP_ID_DPORT_MASK);

		/* Break if we found an unused ephemeral port */
		if (conn == NULL)
			break;
	}

	/* Post sport lock */
	csp_bin_sem_post(&sport_lock);

	/* If no available ephemeral port was found */
	if (sport == start)
		return NULL;

	/* Get storage for new connection */
	conn = csp_conn_new(incoming_id, outgoing_id);
	if (conn == NULL)
		return NULL;

	/* Set connection options */
	conn->opts = opts;

#ifdef CSP_USE_RDP
	/* Call Transport Layer connect */
	if (outgoing_id.flags & CSP_FRDP) {
		/* If the transport layer has failed to connect
		 * deallocate connection structure again and return NULL */
		if (csp_rdp_connect(conn, timeout) != CSP_ERR_NONE) {
			csp_close(conn);
			return NULL;
		}
	}
#endif

	/* We have a successful connection */
	return conn;

}

inline int csp_conn_dport(csp_conn_t * conn) {

	return conn->idin.dport;

}

inline int csp_conn_sport(csp_conn_t * conn) {

	return conn->idin.sport;

}

inline int csp_conn_dst(csp_conn_t * conn) {

	return conn->idin.dst;

}

inline int csp_conn_src(csp_conn_t * conn) {

	return conn->idin.src;

}

inline int csp_conn_flags(csp_conn_t * conn) {

	return conn->idin.flags;

}

#ifdef CSP_DEBUG
void csp_conn_print_table(void) {

	int i;
	csp_conn_t * conn;

	for (i = 0; i < CSP_CONN_MAX; i++) {
		conn = &arr_conn[i];
		printf("[%02u %p] S:%u, %u -> %u, %u -> %u, sock: %p\n",
				i, conn, conn->state, conn->idin.src, conn->idin.dst,
				conn->idin.dport, conn->idin.sport, conn->socket);
#ifdef CSP_USE_RDP
		if (conn->idin.flags & CSP_FRDP)
			csp_rdp_conn_print(conn);
#endif
	}
}

int csp_conn_print_table_str(char * str_buf, int str_size) {

	int i, start = 0;
	csp_conn_t * conn;
	char buf[100];

	/* Display up to 10 connections */
	if (CSP_CONN_MAX - 10 > 0)
		start = CSP_CONN_MAX - 10;

	for (i = start; i < CSP_CONN_MAX; i++) {
		conn = &arr_conn[i];
		snprintf(buf, sizeof(buf), "[%02u %p] S:%u, %u -> %u, %u -> %u, sock: %p\n",
				i, conn, conn->state, conn->idin.src, conn->idin.dst,
				conn->idin.dport, conn->idin.sport, conn->socket);

		strncat(str_buf, buf, str_size);
		if ((str_size -= strlen(buf)) <= 0)
			break;
	}

	return CSP_ERR_NONE;

}
#endif
