/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

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

/* Static connection pool and lock */
static csp_conn_t arr_conn[CSP_CONN_MAX];

static csp_bin_sem_handle_t conn_lock;

/** csp_conn_timeout
 * Walk trough open connections and check if anything needs to be
 * kicked, closed or retransmitted.
 */
void csp_conn_check_timeouts(void) {
#if CSP_USE_RDP
	/* Loop */
	int i;
	for (i = 0; i < CSP_CONN_MAX; i++) {

		/* Only look at open connetions */
		if (arr_conn[i].state != CONN_OPEN)
			continue;

		/* Check the protocol and higher layers */
		switch (arr_conn[i].idin.protocol) {
		case CSP_RDP:
			csp_rdp_check_timeouts(&arr_conn[i]);
		}
	}
#endif
}

/** csp_conn_init
 * Initialises the connection pool
 */
void csp_conn_init(void) {

	int i;
	for (i = 0; i < CSP_CONN_MAX; i++) {
        arr_conn[i].rx_queue = csp_queue_create(CSP_CONN_QUEUE_LENGTH, sizeof(csp_packet_t *));
		arr_conn[i].state = CONN_CLOSED;
		arr_conn[i].l4data = NULL;
	}

	if (csp_bin_sem_create(&conn_lock) != CSP_SEMAPHORE_OK)
		csp_debug(CSP_ERROR, "No more memory for conn semaphore\r\n");

}

/** csp_conn_find
 * Used by the incoming data handler this function searches
 * for an already established connection with a given incoming identifier.
 * The mask field is used to select which parts of the identifier that constitute a
 * unique connection
 * 
 * @return A connection pointer to the matching connection or NULL if no matching connection was found
 */
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

/** csp_conn_new
 * Finds an unused conn or creates a conn 
 * 
 * @return a pointer to the newly established connection or NULL
 */
csp_conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout) {

	static uint8_t csp_conn_last_given = 0;
	int i;
	csp_conn_t * conn;

	/* Search for free connection */
	i = csp_conn_last_given;								// Start with the last given element
	i = (i + 1) % CSP_CONN_MAX;									// Increment by one

	if (csp_bin_sem_wait(&conn_lock, 100) != CSP_SEMAPHORE_OK) {
		csp_debug(CSP_ERROR, "Failed to lock conn array\r\n");
		return NULL;
	}

	do {
		conn = &arr_conn[i];
		if (conn->state == CONN_CLOSED) {
			conn->state = CONN_OPEN;
            break;
        }
		i = (i + 1) % CSP_CONN_MAX;
	} while(i != csp_conn_last_given);

	csp_bin_sem_post(&conn_lock);

	if (i == csp_conn_last_given) {
		csp_debug(CSP_ERROR, "No more free connections\r\n");
		return NULL;
	}

	csp_conn_last_given = i;

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

    /* Ensure l4 knows this conn is opening */
	int result = 1;

#if CSP_USE_RDP
    switch(conn->idin.protocol) {
	case CSP_RDP:
		result = csp_rdp_allocate(conn);
		break;
	}
#endif
    
    if (result == 0) {
    	conn->state = CONN_CLOSED;
    	return NULL;
    }

    return conn;

}

/** csp_close_wait
 * This function must be called whenever the network stack wants to close the connection.
 * The philosophy is that only the "owner" of the connection handle, which is the task,
 * may close the connection.
 * Sometimes when the network stack has a new connection but not yet passed this to userspace,
 * the network stack should call csp_close directly, otherwise it should call csp_close_wait.
 * This function posts a null pointer to the qonnection RX queue, this will make the userspace
 * application close the connection.
 * @param conn pointer to connetion structure
 */
void csp_close_wait(csp_conn_t * conn) {

	/* Set state */
	conn->state = CONN_CLOSE_WAIT;

    /* Try to wake any tasks */
    void * null_pointer = NULL;
    csp_queue_enqueue(conn->rx_queue, &null_pointer, 0);

}

/** csp_close
 * Closes a given connection and frees the buffer if more than 8 bytes
 * if the connection uses an outgoing port this port must also be closed
 * A dynamically allocated connection must be freed.
 */

void csp_close(csp_conn_t * conn) {

	if (conn == NULL) {
		csp_debug(CSP_ERROR, "NULL Pointer given to csp_close\r\n");
		return;
	}

	if (conn->state == CONN_CLOSED) {
		csp_debug(CSP_BUFFER, "Conn already closed by transport layer\r\n");
		return;
	}

	/* Ensure connection queue is empty */
	csp_packet_t * packet;
    while(csp_queue_dequeue(conn->rx_queue, &packet, 0) == CSP_QUEUE_OK) {
    	if (packet != NULL)
    		csp_buffer_free(packet);
    }

    /* Ensure l4 knows this conn is closing */
#if CSP_USE_RDP
    switch(conn->idin.protocol) {
    case CSP_RDP:
		csp_rdp_close(conn);
		break;
	}
#endif

    /* Set to closed */
    conn->state = CONN_CLOSED;

}

/** csp_connect
 * Used to establish outgoing connections
 * This function searches the port table for free slots and finds an unused
 * connection from the connection pool
 * There is no handshake in the CSP protocol
 * @return a pointer to a new connection or NULL
 */
csp_conn_t * csp_connect(csp_protocol_t protocol, uint8_t prio, uint8_t dest, uint8_t dport, unsigned int timeout) {

#if CSP_RANDOMIZE_EPHEM
	static uint8_t sport = (rand() % (CSP_ID_PORT_MAX - CSP_MAX_BIND_PORT)) + (CSP_MAX_BIND_PORT + 1);
#else
	static uint8_t sport = CSP_MAX_BIND_PORT + 1;
#endif
    
	/* Generate CAN identifier */
	csp_id_t incoming_id, outgoing_id;
	incoming_id.pri = prio;
	incoming_id.dst = my_address;
	incoming_id.src = dest;
	incoming_id.sport = dport;
	incoming_id.protocol = protocol;
	outgoing_id.pri = prio;
	outgoing_id.dst = dest;
	outgoing_id.src = my_address;
	outgoing_id.dport = dport;
	outgoing_id.protocol = protocol;
    
    /* Find an unused ephemeral port */
    csp_conn_t * conn;
    
    uint8_t start = sport;

    while (++sport != start) {
        if (sport > CSP_ID_PORT_MAX)
            sport = CSP_MAX_BIND_PORT + 1;

        outgoing_id.sport = sport;
        incoming_id.dport = sport;
        
        /* Match on destination port of _incoming_ identifier */
        conn = csp_conn_find(incoming_id.ext, CSP_ID_DPORT_MASK);

        /* Break if we found an unused ephem port */
        if (conn == NULL)
            break;
    }

    /* If no available ephemeral port was found */
    if (sport == start)
        return NULL;

    /* Get storage for new connection */
    conn = csp_conn_new(incoming_id, outgoing_id);
    if (conn == NULL)
    	return NULL;

    /* Call Transport Layer connect */
    int result = 1;
#if CSP_USE_RDP
    switch(protocol) {
    case CSP_RDP:
    	result = csp_rdp_connect_active(conn, timeout);
    	break;
    default:
    	break;
    }
#endif

    /* If the transport layer has failed to connect
     * deallocate connetion structure again and return NULL
     */
    if (result == 0) {
    	csp_close(conn);
		return NULL;
    }

    /* We have a successfull connection */
    return conn;

}

#if CSP_DEBUG
/**
 * Small helper function to display the connection table
 */
void csp_conn_print_table(void) {

	int i;
	csp_conn_t * conn;

    for (i = 0; i < CSP_CONN_MAX; i++) {
		conn = &arr_conn[i];
		printf("[%02u %p] S:%u, %u -> %u, %u -> %u, sock: %p\r\n", i, conn, conn->state, conn->idin.src, conn->idin.dst, conn->idin.dport, conn->idin.sport, conn->rx_socket);

#if CSP_USE_RDP
		switch(conn->idin.protocol) {
		case CSP_RDP:
			csp_rdp_conn_print(conn);
			break;
		}
#endif

    }
}
#endif

/**
 * @param conn pointer to connection structure
 * @return destination port of an incoming connection
 */
inline int csp_conn_dport(csp_conn_t * conn) {

    return conn->idin.dport;

}

/**
 * @param conn pointer to connection structure
 * @return source port of an incoming connection
 */
inline int csp_conn_sport(csp_conn_t * conn) {

    return conn->idin.sport;

}

/**
 * @param conn pointer to connection structure
 * @return destination address of an incoming connection
 */
inline int csp_conn_dst(csp_conn_t * conn) {

    return conn->idin.dst;

}

/**
 * @param conn pointer to connection structure
 * @return source address of an incoming connection
 */
inline int csp_conn_src(csp_conn_t * conn) {

    return conn->idin.src;

}

/**
 * @param conn pointer to connection structure
 * @return Protocol of incomming connection
 */
inline int csp_conn_protocol(csp_conn_t * conn) {

    return conn->idin.protocol;

}
