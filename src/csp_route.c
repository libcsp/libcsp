#include <csp/csp.h>

#include <stdlib.h>

#include <csp/csp_crc32.h>
#include <csp/arch/csp_endian.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_queue.h>
#include <csp/crypto/csp_hmac.h>
#include <csp/csp_id.h>

#include "csp_port.h"
#include "csp_conn.h"
#include "csp_io.h"
#include "csp_promisc.h"
#include "csp_qfifo.h"
#include "csp_dedup.h"
#include "csp_rdp.h"
#include <csp/csp_debug.h>
#include <csp/csp_iflist.h>
#include "csp_macro.h"

#if CSP_TRACEROUTE
#include <csp/csp_traceroute.h>
#endif

#if ENABLE_ON_CONNECT_SOCKET_CALLBACK
/**
 * on connect callback function holder
 */
static csp_socket_on_connect_callback_t on_connect_socket_callback = NULL;

/**
 * Set the on connect callback function
 *
 * @param[in] callback callback function to be called when a connection is established
 */
void csp_set_on_connect_callback(csp_socket_on_connect_callback_t callback)
{
#if DEBUG
    if (on_connect_socket_callback != NULL) {
		csp_print(CSP_LL_INFO, "csp_set_on_connect_callback: callback already set, overwriting.\n");
	}
#endif
    on_connect_socket_callback = callback;
}
#endif // ENABLE_ON_CONNECT_SOCKET_CALLBACK

/**
 * Check supported packet options
 * @param iface pointer to incoming interface
 * @param packet pointer to packet
 * @return CSP_ERR_NONE is all options are supported, CSP_ERR_NOTSUP if not
 */
static int csp_route_check_options(csp_iface_t * iface, csp_packet_t * packet) {
	/* Avoid compiler warnings about unused parameter */
	(void)iface;
	(void)packet;

#if (CSP_USE_HMAC == 0)
	/* Drop HMAC packets */
	if (packet->id.flags & CSP_FHMAC) {
		csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
		iface->autherr++;
#if CSP_TRACEROUTE
		if (packet->id.flags & CSP_FTRACE) {
			csp_traceroute_log_hop(&packet->id, iface, 2, CSP_TRACE_DROP_UNSUP_HMAC, CSP_NO_VIA_ADDRESS);
		}
#endif
		return CSP_ERR_NOTSUP;
	}
#endif

#if (CSP_USE_RDP == 0)
	/* Drop RDP packets */
	if (packet->id.flags & CSP_FRDP) {
		csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
		iface->rx_error++;
#if CSP_TRACEROUTE
		if (packet->id.flags & CSP_FTRACE) {
			csp_traceroute_log_hop(&packet->id, iface, 2, CSP_TRACE_DROP_UNSUP_RDP, CSP_NO_VIA_ADDRESS);
		}
#endif
		return CSP_ERR_NOTSUP;
	}
#endif
	return CSP_ERR_NONE;
}

/**
 * Helper function to decrypt, check auth and CRC32
 * @param security_opts either socket_opts or conn_opts
 * @param iface pointer to incoming interface
 * @param packet pointer to packet
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
static int csp_route_security_check(uint32_t security_opts, csp_iface_t * iface, csp_packet_t * packet) {


	/* CRC32 verified packet */
	if (packet->id.flags & CSP_FCRC32) {
		/* Verify CRC32 (does not include header for backwards compatability with csp1.x) */
		if (csp_crc32_verify(packet) != CSP_ERR_NONE) {
			iface->rx_error++;
#if CSP_TRACEROUTE
			if (packet->id.flags & CSP_FTRACE) {
				csp_traceroute_log_hop(&packet->id, iface, 2, CSP_TRACE_DROP_CRC32_FAIL, CSP_NO_VIA_ADDRESS);
			}
#endif
			return CSP_ERR_CRC32;
		}
	} else if (security_opts & CSP_SO_CRC32REQ) {
		iface->rx_error++;
#if CSP_TRACEROUTE
		if (packet->id.flags & CSP_FTRACE) {
			csp_traceroute_log_hop(&packet->id, iface, 2, CSP_TRACE_DROP_CRC32_REQ, CSP_NO_VIA_ADDRESS);
		}
#endif
		return CSP_ERR_CRC32;
	}

#if (CSP_USE_HMAC)
	/* HMAC authenticated packet */
	if (packet->id.flags & CSP_FHMAC) {
		/* Verify HMAC (does not include header for backwards compatability with csp1.x) */
		if (csp_hmac_verify(packet, false) != CSP_ERR_NONE) {
			/* HMAC failed */
			iface->autherr++;
#if CSP_TRACEROUTE
			if (packet->id.flags & CSP_FTRACE) {
				csp_traceroute_log_hop(&packet->id, iface, 2, CSP_TRACE_DROP_HMAC_FAIL, CSP_NO_VIA_ADDRESS);
			}
#endif
			return CSP_ERR_HMAC;
		}
	} else if (security_opts & CSP_SO_HMACREQ) {
		iface->autherr++;
#if CSP_TRACEROUTE
		if (packet->id.flags & CSP_FTRACE) {
			csp_traceroute_log_hop(&packet->id, iface, 2, CSP_TRACE_DROP_HMAC_REQ, CSP_NO_VIA_ADDRESS);
		}
#endif
		return CSP_ERR_HMAC;
	}
#endif

#if (CSP_USE_RDP)
	/* RDP packet */
	if (!(packet->id.flags & CSP_FRDP)) {
		if (security_opts & CSP_SO_RDPREQ) {
			iface->rx_error++;
#if CSP_TRACEROUTE
			if (packet->id.flags & CSP_FTRACE) {
				csp_traceroute_log_hop(&packet->id, iface, 2, CSP_TRACE_DROP_RDP_REQ, CSP_NO_VIA_ADDRESS);
			}
#endif
			return CSP_ERR_INVAL;
		}
	}
#endif

	return CSP_ERR_NONE;
}


__weak void csp_input_hook(csp_iface_t * iface, csp_packet_t * packet) {
	csp_print_packet("INP: S %u, D %u, Dp %u, Sp %u, Pr %u, Fl 0x%02X, Sz %" PRIu16 " VIA: %s, Tms %u\n",
					 packet->id.src, packet->id.dst, packet->id.dport,
					 packet->id.sport, packet->id.pri, packet->id.flags, packet->length, iface->name, csp_get_ms());

#if CSP_TRACEROUTE
	/* Log trace hop if trace flag is set */
	if (packet->id.flags & CSP_FTRACE) {
		csp_traceroute_log_hop(&packet->id, iface, 0, CSP_TRACE_RECEIVED, CSP_NO_VIA_ADDRESS);
	}
#endif
}

int csp_route_work(void) {

	csp_qfifo_t input;
	csp_packet_t * packet;
	csp_conn_t * conn;
	csp_socket_t * socket;

#if (CSP_USE_RDP)
	/* Check connection timeouts (currently only for RDP) */
	csp_conn_check_timeouts();
#endif

	/* Get next packet to route */
	if (csp_qfifo_read(&input) != CSP_ERR_NONE) {
		return CSP_ERR_TIMEDOUT;
	}

	packet = input.packet;
	if (packet == NULL) {
		return CSP_ERR_TIMEDOUT;
	}

	csp_input_hook(input.iface, packet);

	/* Count the message */
	input.iface->rx++;
	input.iface->rxbytes += packet->length;

	/* The packet is to me, if the address matches that of any interface,
	 * or the address matches the broadcast address of the incoming interface */
	int is_to_me = (csp_iflist_get_by_addr(packet->id.dst) != NULL || (csp_id_is_broadcast(packet->id.dst, input.iface)));

	/* Deduplication */
	if ((csp_conf.dedup == CSP_DEDUP_ALL) ||
		((is_to_me) && (csp_conf.dedup == CSP_DEDUP_INCOMING)) ||
		((!is_to_me) && (csp_conf.dedup == CSP_DEDUP_FWD))) {
		if (csp_dedup_is_duplicate(packet)) {
			csp_print(CSP_LL_TRACE, "Deduplication: Discarding duplicate packet (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
				  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);

			/* Discard packet */
			input.iface->drop++;
#if CSP_TRACEROUTE
			if (packet->id.flags & CSP_FTRACE) {
				csp_traceroute_log_hop(&packet->id, input.iface, 2, CSP_TRACE_DROP_DUPLICATE, CSP_NO_VIA_ADDRESS);
			}
#endif
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}
	}

	/* Here there be promiscuous mode */
#if (CSP_USE_PROMISC)
	csp_promisc_add(packet);
#endif

	/* If the message is not to me, route the message to the correct interface */
	if (!is_to_me) {
		csp_print(CSP_LL_TRACE, "Routing: Forwarding packet to next hop (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
				  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
		csp_send_direct(&packet->id, packet, input.iface);
		return CSP_ERR_NONE;

	}

	/* Discard packets with unsupported options */
	if (csp_route_check_options(input.iface, packet) != CSP_ERR_NONE) {
		csp_print(CSP_LL_TRACE, "Routing: Discarding packet with unsupported options (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
				  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
		csp_buffer_free(packet);
		return CSP_ERR_NONE;
	}

#if CSP_TRACEROUTE
	/* Log DELIVERED event - packet has reached its destination */
	if (packet->id.flags & CSP_FTRACE) {
		csp_traceroute_log_hop(&packet->id, input.iface, 0, CSP_TRACE_DELIVERED, CSP_NO_VIA_ADDRESS);
	}
#endif

	/**
	 * Callbacks
	 */
	csp_callback_t callback = csp_port_get_callback(packet->id.dport);
	if (callback) {
		if (csp_route_security_check(CSP_SO_CRC32REQ, input.iface, packet) != CSP_ERR_NONE) {
			csp_print(CSP_LL_TRACE, "Routing: Discarding packet - failed security check (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
					  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		callback(packet);
		return CSP_ERR_NONE;
	}

	/**
	 * Sockets
	 */

	/* The message is to me, search for incoming socket */
	socket = csp_port_get_socket(packet->id.dport);

	/* If the socket is connection-less, deliver now */
	if (socket && (socket->opts & CSP_SO_CONN_LESS)) {

		if (csp_route_security_check(socket->opts, input.iface, packet) != CSP_ERR_NONE) {
			csp_print(CSP_LL_TRACE, "Routing: Discarding packet - failed security check (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
					  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		if (csp_queue_enqueue(socket->rx_queue, &packet, 0) != CSP_QUEUE_OK) {
			csp_print(CSP_LL_TRACE, "Routing: Discarding packet - socket queue is full (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
					  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
			csp_dbg_conn_ovf++;
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		return CSP_ERR_NONE;
	}

	/* Search for an existing connection */
	conn = csp_conn_find_existing(&packet->id);
	if (NULL == conn) { // If this is an incoming packet on a new connection
		/* Reject packet if no matching socket is found */
		if (!socket) {
			csp_print(CSP_LL_TRACE, "Routing: Discarding packet - no matching socket found (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
					  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		/* Run security check on incoming packet */
		if (csp_route_security_check(socket->opts, input.iface, packet) != CSP_ERR_NONE) {
			csp_print(CSP_LL_TRACE, "Routing: Discarding packet - failed security check (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
					  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		/* New incoming connection accepted */
		csp_id_t idout;
		idout.pri = packet->id.pri;
		idout.src = packet->id.dst;
		idout.dst = packet->id.src;
		idout.dport = packet->id.sport;
		idout.sport = packet->id.dport;
		idout.flags = packet->id.flags;

		/* Create connection */
		conn = csp_conn_new(packet->id, idout, CONN_SERVER);
		if (!conn) {
			csp_print(CSP_LL_TRACE, "Routing: Discarding packet - failed to create connection (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
					  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
			csp_dbg_conn_out++;
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		/* Store the socket queue and options */
		conn->dest_socket = socket;
		conn->opts = socket->opts;

	} else {  // Packet is to existing connection */
		/* Run security check on incoming packet */
		if (csp_route_security_check(conn->opts, input.iface, packet) != CSP_ERR_NONE) {
			csp_print(CSP_LL_TRACE, "Routing: Discarding packet - failed security check (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
					  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}
	}

#if (CSP_USE_RDP)
	/* Pass packet to RDP module */
	if (packet->id.flags & CSP_FRDP) {
		csp_print(CSP_LL_TRACE, "Routing: Passing packet to RDP (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
				  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
		bool close_connection = csp_rdp_new_packet(conn, packet);
		if (close_connection) {
			csp_close(conn);
		}
		return CSP_ERR_NONE;
	}
#endif

	/* Otherwise, enqueue directly */
	if (csp_conn_enqueue_packet(conn, packet) != CSP_ERR_NONE) {
		csp_print(CSP_LL_TRACE, "Routing: Discarding packet - connection queue is full (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
				  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
		csp_dbg_conn_ovf++;
		csp_buffer_free(packet);
		return CSP_ERR_NONE;
	}

	/* Try to queue up the new connection pointer */
	if (conn->dest_socket != NULL) {
		if (csp_queue_enqueue(conn->dest_socket->rx_queue, &conn, 0) != CSP_QUEUE_OK) {
			csp_print(CSP_LL_TRACE, "Routing: Discarding packet - socket queue is full (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
					  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
			csp_dbg_conn_ovf++;
			csp_close(conn);
			return CSP_ERR_NONE;
		}

#if ENABLE_ON_CONNECT_SOCKET_CALLBACK
		if (on_connect_socket_callback != NULL) {
			on_connect_socket_callback(conn->dest_socket);
		}
#endif  // ENABLE_ON_CONNECT_SOCKET_CALLBACK

		/* Ensure that this connection will not be posted to this socket again */
		conn->dest_socket = NULL;
	}

	csp_print(CSP_LL_TRACE, "Routing: Successfully delivered a packet (src=%u dst=%u dp=%u sp=%u iface=%s)\n",
			  packet->id.src, packet->id.dst, packet->id.dport, packet->id.sport, input.iface->name);
	return CSP_ERR_NONE;
}
