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

#include "csp_buffer.h"
#include "csp_conn.h"

/* Static connection pool and lock */
static csp_conn_t arr_conn[MAX_STATIC_CONNS];

/** csp_conn_init
 * Initialises the connection pool
 */
void csp_conn_init(void) {

	int i;
	for (i = 0; i < MAX_STATIC_CONNS; i++) {
        arr_conn[i].rx_queue = csp_queue_create(20, sizeof(csp_packet_t *));
		arr_conn[i].state = SOCKET_CLOSED;
	}

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

    for (i = 0; i < MAX_STATIC_CONNS; i++) {
		conn = &arr_conn[i];
		if(((conn->idin.ext & mask) == (id & mask)) && (conn->state != SOCKET_CLOSED))
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

    /* Search for free connection */
    int i;
    csp_conn_t * conn;

    CSP_ENTER_CRITICAL();
	for (i = 0; i < MAX_STATIC_CONNS; i++) {
		conn = &arr_conn[i];

		if(conn->state == SOCKET_CLOSED) {
			conn->state = SOCKET_OPEN;
            conn->idin = idin;
            conn->idout = idout;
            CSP_EXIT_CRITICAL();
            return conn;
        }
    }
	CSP_EXIT_CRITICAL();
    
    return NULL;
  
}

/** csp_close
 * Closes a given connection and frees the buffer if more than 8 bytes
 * if the connection uses an outgoing port this port must also be closed
 * A dynamically allocated connection must be freed.
 */

void csp_close(csp_conn_t * conn) {
   
	/* Ensure connection queue is empty */
	csp_packet_t * packet;
    while(csp_queue_dequeue(conn->rx_queue, &packet, 0) == CSP_QUEUE_OK)
    	csp_buffer_free(packet);

    /* Set to closed */
    conn->state = SOCKET_CLOSED;

}

/** csp_connect
 * Used to establish outgoing connections
 * This function searches the port table for free slots and finds an unused
 * connection from the connection pool
 * There is no handshake in the CSP protocol
 * @return a pointer to a new connection or NULL
 */
csp_conn_t * csp_connect(uint8_t prio, uint8_t dest, uint8_t dport) {

	uint8_t sport = 31;

	/* Generate CAN identifier */
	csp_id_t incoming_id, outgoing_id;
	incoming_id.pri = prio;
	incoming_id.dst = my_address;
	incoming_id.src = dest;
	incoming_id.sport = dport;
	outgoing_id.pri = prio;
	outgoing_id.dst = dest;
	outgoing_id.src = my_address;
	outgoing_id.dport = dport;
    
    /* Find an unused ephemeral port */
    csp_conn_t * conn;
    while (sport > 17) {
	    outgoing_id.sport = sport;
        incoming_id.dport = sport;
        /* Match on source port */
        conn = csp_conn_find(incoming_id.ext, 0x00001F00);
        /* If no connection with this identifier was found,
         * go ahead and use sport as outgoing port */
        if (conn == NULL) {
            break;
        } else {
            sport--;
        }
    }

    conn = csp_conn_new(incoming_id, outgoing_id);

    return conn;
}

