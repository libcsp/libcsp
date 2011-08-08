/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk) 

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

/* CSP includes */
#include <csp/csp.h>

#include "arch/csp_thread.h"
#include "arch/csp_queue.h"
#include "arch/csp_semaphore.h"
#include "arch/csp_malloc.h"
#include "arch/csp_time.h"

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
#if CSP_USE_RDP
	/* Loop */
	int i;
	for (i = 0; i < CSP_CONN_MAX; i++) {

		/* Only look at open connetions */
		if (arr_conn[i].state != CONN_OPEN)
			continue;

		/* Check the protocol and higher layers */
		if (arr_conn[i].idin.flags & CSP_FRDP)
			csp_rdp_check_timeouts(&arr_conn[i]);

	}
#endif
}

void csp_conn_init(void) {

	/* Initialize source port */
#if CSP_RANDOMIZE_EPHEM
	srand(csp_get_ms());
	sport = (rand() % (CSP_ID_PORT_MAX - CSP_MAX_BIND_PORT)) + (CSP_MAX_BIND_PORT + 1);
#else
	sport = CSP_MAX_BIND_PORT + 1;
#endif

	if (csp_bin_sem_create(&sport_lock) != CSP_SEMAPHORE_OK) {
		csp_debug(CSP_ERROR, "No more memory for sport semaphore\r\n");
		return;
	}

	int i;
	for (i = 0; i < CSP_CONN_MAX; i++) {
        arr_conn[i].rx_queue = csp_queue_create(CSP_CONN_QUEUE_LENGTH, sizeof(csp_packet_t *));
		arr_conn[i].state = CONN_CLOSED;
#if CSP_USE_RDP
		if (csp_rdp_allocate(&arr_conn[i]) == 0)
			csp_debug(CSP_ERROR, "Failed to create queues for RDP in csp_conn_init\r\n");
#endif
	}

	if (csp_bin_sem_create(&conn_lock) != CSP_SEMAPHORE_OK) {
		csp_debug(CSP_ERROR, "No more memory for conn semaphore\r\n");
		return;
	}

}

csp_conn_t * csp_conn_find(uint32_t id, uint32_t mask) {

	/* Search for matching connection */
	int i;
	csp_conn_t * conn;

    for (i = 0; i < CSP_CONN_MAX; i++) {
		conn = &arr_conn[i];
		if ((conn->state != CONN_CLOSED) && (conn->idin.ext & mask) == (id & mask))
			return conn;
    }
    
    return NULL;

}

csp_conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout) {

	static uint8_t csp_conn_last_given = 0;
	int i;
	csp_conn_t * conn;

	if (csp_bin_sem_wait(&conn_lock, 100) != CSP_SEMAPHORE_OK) {
		csp_debug(CSP_ERROR, "Failed to lock conn array\r\n");
		return NULL;
	}

	/* Search for free connection */
	i = csp_conn_last_given;								// Start with the last given element
	i = (i + 1) % CSP_CONN_MAX;									// Increment by one

	do {
		conn = &arr_conn[i];
		if (conn->state == CONN_CLOSED) {
			conn->state = CONN_OPEN;
            break;
        }
		i = (i + 1) % CSP_CONN_MAX;
	} while(i != csp_conn_last_given);

	if (i == csp_conn_last_given) {
		csp_debug(CSP_ERROR, "No more free connections\r\n");
		csp_bin_sem_post(&conn_lock);
		return NULL;
	}

	csp_conn_last_given = i;
	csp_bin_sem_post(&conn_lock);

	/* No lock is needed here, because nobody else
	 * has a reference to this connection yet.
	 */
	conn->idin = idin;
	conn->idout = idout;
	conn->rx_socket = NULL;
	conn->open_timestamp = csp_get_ms();

	/* Ensure connection queue is empty */
	csp_packet_t * packet;
	while(csp_queue_dequeue(conn->rx_queue, &packet, 0) == CSP_QUEUE_OK) {
		if (packet != NULL)
			csp_buffer_free(packet);
	}

    return conn;

}

void csp_close(csp_conn_t * conn) {

	if (conn == NULL) {
		csp_debug(CSP_ERROR, "NULL Pointer given to csp_close\r\n");
		return;
	}

	if (conn->state == CONN_CLOSED) {
		csp_debug(CSP_BUFFER, "Conn already closed by transport layer\r\n");
		return;
	}

    /* Ensure l4 knows this conn is closing */
#if CSP_USE_RDP
    if (conn->idin.flags & CSP_FRDP || conn->idout.flags & CSP_FRDP)
		if (csp_rdp_close(conn) == 1)
			return;
#endif

    /* Lock connection array while closing connection */
    if (csp_bin_sem_wait(&conn_lock, 100) != CSP_SEMAPHORE_OK) {
		csp_debug(CSP_ERROR, "Failed to lock conn array\r\n");
		return;
	}

    /* Set to closed */
	conn->state = CONN_CLOSED;

	/* Ensure connection queue is empty */
	csp_packet_t * packet;
    while(csp_queue_dequeue(conn->rx_queue, &packet, 0) == CSP_QUEUE_OK) {
    	if (packet != NULL)
    		csp_buffer_free(packet);
    }

    /* Reset RDP state */
#if CSP_USE_RDP
    if (conn->idin.flags & CSP_FRDP)
    	csp_rdp_flush_all(conn);
#endif

    /* Unlock connection array */
    csp_bin_sem_post(&conn_lock);
}

csp_conn_t * csp_connect(uint8_t prio, uint8_t dest, uint8_t dport, unsigned int timeout, uint32_t opts) {

	/* Generate identifier */
	csp_id_t incoming_id, outgoing_id;
	incoming_id.pri = prio;
	incoming_id.dst = my_address;
	incoming_id.src = dest;
	incoming_id.sport = dport;
	incoming_id.flags = 0;
	outgoing_id.pri = prio;
	outgoing_id.dst = dest;
	outgoing_id.src = my_address;
	outgoing_id.dport = dport;
	outgoing_id.flags = 0;

	/* Set connection options */
	if (opts & CSP_O_RDP) {
#if CSP_USE_RDP
		incoming_id.flags |= CSP_FRDP;
		outgoing_id.flags |= CSP_FRDP;
#else
		csp_debug(CSP_ERROR, "Attempt to create RDP connection, but CSP was compiled without RDP support\r\n");
		return NULL;
#endif
	}

	if (opts & CSP_O_HMAC) {
#if CSP_ENABLE_HMAC
		outgoing_id.flags |= CSP_FHMAC;
		incoming_id.flags |= CSP_FHMAC;
#else
		csp_debug(CSP_ERROR, "Attempt to create HMAC authenticated connection, but CSP was compiled without HMAC support\r\n");
		return NULL;
#endif
	}

	if (opts & CSP_O_XTEA) {
#if CSP_ENABLE_XTEA
		outgoing_id.flags |= CSP_FXTEA;
		incoming_id.flags |= CSP_FXTEA;
#else
		csp_debug(CSP_ERROR, "Attempt to create XTEA encrypted connection, but CSP was compiled without XTEA support\r\n");
		return NULL;
#endif
	}

	if (opts & CSP_O_CRC32) {
#if CSP_ENABLE_CRC32
		outgoing_id.flags |= CSP_FCRC32;
		incoming_id.flags |= CSP_FCRC32;
#else
		csp_debug(CSP_ERROR, "Attempt to create CRC32 validated connection, but CSP was compiled without CRC32 support\r\n");
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
    conn->conn_opts = opts;

    /* Call Transport Layer connect */
    int result = 1;
#if CSP_USE_RDP
    if (outgoing_id.flags & CSP_FRDP)
       	result = csp_rdp_connect_active(conn, timeout);
#endif

    /* If the transport layer has failed to connect
     * deallocate connection structure again and return NULL
     */
    if (result == 0) {
    	csp_close(conn);
		return NULL;
    }

    /* We have a successful connection */
    return conn;

}

#if CSP_DEBUG
void csp_conn_print_table(void) {

	int i;
	csp_conn_t * conn;

    for (i = 0; i < CSP_CONN_MAX; i++) {
		conn = &arr_conn[i];
		printf("[%02u %p] S:%u, %u -> %u, %u -> %u, sock: %p\r\n",
				i, conn, conn->state, conn->idin.src, conn->idin.dst,
				conn->idin.dport, conn->idin.sport, conn->rx_socket);
#if CSP_USE_RDP
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
		snprintf(buf, sizeof(buf), "[%02u %p] S:%u, %u -> %u, %u -> %u, sock: %p\r\n",
				i, conn, conn->state, conn->idin.src, conn->idin.dst,
				conn->idin.dport, conn->idin.sport, conn->rx_socket);

		strncat(str_buf, buf, str_size);
        if ((str_size -= strlen(buf)) <= 0)
            break;
    }

    return 0;

}
#endif

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
