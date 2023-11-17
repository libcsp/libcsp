#include "csp/autoconfig.h"

#include "csp_conn.h"

#include <stdlib.h>
#include <stdatomic.h>

#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_time.h>
#include <csp/csp_id.h>
#include <csp/csp_debug.h>
#include "csp_macro.h"
#include "csp_rdp_queue.h"
#include "csp_rdp.h"

#define OUTGOING_PORTS (((1 << (CSP_ID2_PORT_SIZE)) - 1) - CSP_PORT_MAX_BIND)
#if OUTGOING_PORTS > CSP_CONN_MAX
#error "More connections than available outgoing ports"
#endif

/* Connection pool */
static csp_conn_t arr_conn[CSP_CONN_MAX] __noinit;

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
		csp_dbg_conn_ovf++;
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;
}

void csp_conn_init(void) {

	for (int i = 0; i < CSP_CONN_MAX; i++) {
		csp_conn_t * conn = &arr_conn[i];

		conn->sport_outgoing = CSP_PORT_MAX_BIND + 1 + i;
		conn->state = CONN_CLOSED;
		conn->idin.flags = 0;
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

		/* Connection must be open */
		if (conn->state != CONN_OPEN)
			continue;

		/**
		 * This search looks verbose, Instead of a big if statement, it is written out as
		 * conditions. This has been done for clarity. The least likely check is put first
		 * for runtime speed improvement.
		 * Also this is written using explicit fields, not bitmasks, in order to improve
		 * portability and dual use between different header formats.
		 */

		/* Outgoing connections are uniquely defined by the source port,
		 * So only the incoming destination port must match. This means
		 * that responses to broadcast addresses, are accepted as long
		 * as the incoming port matches the unique source port of the 
		 * connection */
		if (conn->type == CONN_CLIENT) {

			/* Connection must match dport */
			if (conn->idin.dport != id->dport)
				continue;

		/* Incoming connections are uniquely defined by the source amd
		 * destination port, as well as the source node. Incoming
		 * connections can never come from a brodcast address */
		} else {
      
			/* Connection must match dport */
			if (conn->idin.dport != id->dport)
				continue;

			/* Connection must match sport */
			if (conn->idin.sport != id->sport)
				continue;

			/* Connection must match source */
			if (conn->idin.src != id->src)
				continue;

		}

		

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

	/* Search for free connection */
	csp_conn_t * conn = NULL;
	int i = csp_conn_last_given;
	for (int j = 0; j < CSP_CONN_MAX; j++) {
		i = (i + 1) % CSP_CONN_MAX;

		int expected = CONN_CLOSED;
		if (atomic_compare_exchange_strong(&arr_conn[i].state, &expected, CONN_OPEN)) {
			conn = &arr_conn[i];
			csp_conn_last_given = i;
			break;
		}
	}

	if (conn == NULL) {
		csp_dbg_conn_out++;
		return NULL;
	}

	conn->timestamp = 0;
	conn->type = type;
	conn->idin.flags = 0;
	conn->idout.flags = 0;
	return conn;
}

csp_conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout, csp_conn_type_t type) {

	/* Allocate connection structure */
	csp_conn_t * conn = csp_conn_allocate(type);

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
		csp_dbg_errno = CSP_DBG_ERR_ALREADY_CLOSED;
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

	/* Ensure connection queue is empty */
	csp_conn_flush_rx_queue(conn);

	/* Reset RDP state */
#if (CSP_USE_RDP)
	if (conn->idin.flags & CSP_FRDP) {
		csp_rdp_queue_flush(conn);
	}
#endif

	/* Set to closed */
	conn->state = CONN_CLOSED;
	
	return CSP_ERR_NONE;
}

csp_conn_t * csp_connect(uint8_t prio, uint16_t dest, uint8_t dport, uint32_t timeout, uint32_t opts) {

	/* Force options on all connections */
	opts |= csp_conf.conn_dfl_so;
	
	/* Generate identifier */
	csp_id_t incoming_id, outgoing_id;

	/* Use 0 as incoming id (this disables the input filter on destination node)
	 * This means that for this outgoing connection, we accept the answer coming to whatever address
	 * the outgoing interface has. CSP does not support "source address" on outgoing connections 
	 * so the outgoing source address will be automatically applied after outgoing routing 
	 * selects which interface the packet will leavve from */
	incoming_id.dst = 0; 
	outgoing_id.src = 0; 

	incoming_id.pri = prio;
	outgoing_id.pri = prio;
	incoming_id.src = dest;
	outgoing_id.dst = dest;
	incoming_id.sport = dport;
	outgoing_id.dport = dport;
	incoming_id.flags = 0;
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
		csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
		return NULL;
#endif
	}

	if (opts & CSP_O_HMAC) {
#if (CSP_USE_HMAC)
		outgoing_id.flags |= CSP_FHMAC;
		incoming_id.flags |= CSP_FHMAC;
#else
		csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
		return NULL;
#endif
	}

	if (opts & CSP_O_CRC32) {
		outgoing_id.flags |= CSP_FCRC32;
		incoming_id.flags |= CSP_FCRC32;
	}

	/* Find a new connection */
	csp_conn_t * conn = csp_conn_new(incoming_id, outgoing_id, CONN_CLIENT);
	if (conn == NULL) {
		return NULL;
	}

	/* Outgoing connections always use pre-defined source port */
	conn->idout.sport = conn->sport_outgoing;
	conn->idin.dport = conn->sport_outgoing;
	conn->dest_socket = NULL;

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

#if (CSP_ENABLE_CSP_PRINT)

void csp_conn_print_table(void) {

	for (unsigned int i = 0; i < CSP_CONN_MAX; i++) {
		__unused csp_conn_t * conn = &arr_conn[i];
		csp_print("[%02u %p] S:%u, %u -> %u, %u -> %u (%u) fl %x\r\n",
		          i, (void *)conn, conn->state, conn->idin.src, conn->idin.dst,
		          conn->idin.dport, conn->idin.sport, conn->sport_outgoing, conn->idin.flags);
#if (CSP_USE_RDP)
		if (conn->idin.flags & CSP_FRDP) {
			csp_print("\tRDP: S:%d (closed by 0x%x), rcv %u, snd %u, win %" PRIu32 "\n", 
			          conn->rdp.state, conn->rdp.closed_by, conn->rdp.rcv_cur, conn->rdp.snd_una, conn->rdp.window_size);
		}
#endif
	}
}

#endif

#if (CSP_HAVE_STDIO)
#include <stdio.h> // snprintf

int csp_conn_print_table_str(char * str_buf, int str_size) {

	/* Display up to 10 connections */
	unsigned int start = (CSP_CONN_MAX > 10) ? (CSP_CONN_MAX - 10) : 0;

	for (unsigned int i = start; i < CSP_CONN_MAX; i++) {
		csp_conn_t * conn = &arr_conn[i];
		char buf[100];
		snprintf(buf, sizeof(buf), "[%02u %p] S:%u, %u -> %u, %u -> %u (%u)\n",
				 i, (void *)conn, conn->state, conn->idin.src, conn->idin.dst,
				 conn->idin.dport, conn->idin.sport, conn->sport_outgoing);

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

bool csp_conn_is_active(csp_conn_t *conn) {
#if (CSP_USE_RDP)
	if ((conn->idin.flags & CSP_FRDP) || (conn->idout.flags & CSP_FRDP)) {
		/* This is for sure an RDP connection */
		return csp_rdp_conn_is_active(conn);
	}
#endif

	/* Non RDP connections are always "active" */
	return true;
}
