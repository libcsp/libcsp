/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 Gomspace ApS (http://www.gomspace.com)
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

#ifndef _CSP_H_
#define _CSP_H_

/**
   @file
   CSP.
*/

#include <csp/csp_platform.h>
#include <csp/csp_error.h>
#include <csp/csp_debug.h>
#include <csp/csp_buffer.h>
#include <csp/csp_rtable.h>
#include <csp/csp_iflist.h>
#include <csp/csp_sfp.h>
#include <csp/csp_promisc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   CSP configuration.
   @see csp_init()
*/
typedef struct csp_conf_s {

	uint8_t address;		/**< CSP address of the system */

	const char *hostname;		/**< Host name, returned by the #CSP_CMP_IDENT request */
	const char *model;		/**< Model, returned by the #CSP_CMP_IDENT request */
	const char *revision;		/**< Revision, returned by the #CSP_CMP_IDENT request */

	uint8_t conn_max;		/**< Max number of connections. A fixed connection array is allocated by csp_init() */
	uint8_t conn_queue_length;	/**< Max queue length (max queued Rx messages). */
	uint8_t fifo_length;		/**< Length of incoming message queue, used for handover to router task. */
	uint8_t port_max_bind;		/**< Max/highest port for use with csp_bind() */
	uint8_t rdp_max_window;		/**< Max RDP window size */
	uint16_t buffers;		/**< Number of CSP buffers */
	uint16_t buffer_data_size;	/**< Data size of a CSP buffer. Total size will be sizeof(#csp_packet_t) + data_size. */
	uint32_t conn_dfl_so;		/**< Default connection options. Options will always be or'ed onto new connections, see csp_connect() */
} csp_conf_t;

/**
   Get default CSP configuration.
*/
static inline void csp_conf_get_defaults(csp_conf_t * conf) {
	conf->address = 1;
	conf->hostname = "hostname";
	conf->model = "model";
	conf->revision = "resvision";
	conf->conn_max = 10;
	conf->conn_queue_length = 10;
	conf->fifo_length = 25;
	conf->port_max_bind = 24;
	conf->rdp_max_window = 20;
	conf->buffers = 10;
	conf->buffer_data_size = 256;
	conf->conn_dfl_so = CSP_O_NONE;
}

/**
   Initialize CSP.
   This will configure/allocate basic structures.
   @param[in] conf configuration. A shallow copy will be done of the provided configuration, i.e. only copy references to strings/structers.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_init(const csp_conf_t * conf);

/**
   Free allocated resorces in CSP.
   This is intended for testing of CSP, in order to be able re-initialize CSP by calling csp_init() again.
*/
void csp_free_resources(void);

/**
   Get a \a read-only reference to the active CSP configuration.
   @return Active CSP configuration (read-only).
*/
const csp_conf_t * csp_get_conf(void);

/**
   Get the system's own address.
   @return system's own address
*/
uint8_t csp_get_address(void);

/**
   Create a CSP socket endpoint.
   @param[in] opts socket options, see @ref CSP_SOCKET_OPTIONS.
   @return Socket on success, NULL on failure
*/
csp_socket_t *csp_socket(uint32_t opts);

/**
   Wait/accept a new connection.
   @param[in] socket socket to accept connections on, created by calling csp_socket().
   @param[in] timeout timeout in mS to wait for a connection, use #CSP_MAX_TIMEOUT for infinite timeout.
   @return New connection on success, NULL on failure or timeout.
*/
csp_conn_t *csp_accept(csp_socket_t *socket, uint32_t timeout);

/**
   Read packet from a connection.
   This fuction will wait on the connection's RX queue for the specified timeout.
   @param[in] conn connection
   @param[in] timeout timeout in mS to wait for a packet, use #CSP_MAX_TIMEOUT for infinite timeout.
   @return Packet or NULL in case of failure or timeout.
*/
csp_packet_t *csp_read(csp_conn_t *conn, uint32_t timeout);

/**
   Send packet on a connection.
   @param[in] conn connection
   @param[in] packet packet to send
   @param[in] timeout unused as of CSP version 1.6
   @return 1 on success, 0 on failure and the packet must be freed by calling csp_buffer_free()
*/
int csp_send(csp_conn_t *conn, csp_packet_t *packet, uint32_t timeout);

/**
   Change the default priority of the connection and send a packet.
   @note The priority of the connection will be changed. If you need to change it back, call csp_send_prio() again.

   @param[in] prio priority to set on the connection
   @param[in] conn connection
   @param[in] packet packet to send
   @param[in] timeout unused as of CSP version 1.6
   @return 1 on success, 0 on failure and the packet must be freed by calling csp_buffer_free()
 */
int csp_send_prio(uint8_t prio, csp_conn_t *conn, csp_packet_t *packet, uint32_t timeout);

/**
   Perform an entire request & reply transaction.
   Creates a connection, send \a outbuf, wait for reply, copy reply to \a inbuf and close the connection.
   @param[in] prio priority, see #csp_prio_t
   @param[in] dst destination address
   @param[in] dst_port destination port
   @param[in] timeout timeout in mS to wait for a reply
   @param[in] outbuf outgoing data (request)
   @param[in] outlen length of data in \a outbuf (request)
   @param[out] inbuf user provided buffer for receiving data (reply)
   @param[in] inlen length of expected reply, -1 for unknown size (inbuf MUST be large enough), 0 for no reply.
   @param[in] opts connection options, see @ref CSP_CONNECTION_OPTIONS.
   @return 1 or reply size on success, 0 on failure (error, incoming length does not match, timeout)
*/
int csp_transaction_w_opts(uint8_t prio, uint8_t dst, uint8_t dst_port, uint32_t timeout, void *outbuf, int outlen, void *inbuf, int inlen, uint32_t opts);

/**
   Perform an entire request & reply transaction.
   Creates a connection, send \a outbuf, wait for reply, copy reply to \a inbuf and close the connection.
   @param[in] prio priority, see #csp_prio_t
   @param[in] dest destination address
   @param[in] port destination port
   @param[in] timeout timeout in mS to wait for a reply
   @param[in] outbuf outgoing data (request)
   @param[in] outlen length of data in \a outbuf (request)
   @param[out] inbuf user provided buffer for receiving data (reply)
   @param[in] inlen length of expected reply, -1 for unknown size (inbuf MUST be large enough), 0 for no reply.
   @return 1 or reply size on success, 0 on failure (error, incoming length does not match, timeout)
*/
static inline int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, uint32_t timeout, void * outbuf, int outlen, void * inbuf, int inlen) {
	return csp_transaction_w_opts(prio, dest, port, timeout, outbuf, outlen, inbuf, inlen, 0);
}

/**
   Perform an entire request & reply transaction on an existing connection.
   Send \a outbuf, wait for reply and copy reply to \a inbuf.
   @param[in] conn connection
   @param[in] timeout timeout in mS to wait for a reply
   @param[in] outbuf outgoing data (request)
   @param[in] outlen length of data in \a outbuf (request)
   @param[out] inbuf user provided buffer for receiving data (reply)
   @param[in] inlen length of expected reply, -1 for unknown size (inbuf MUST be large enough), 0 for no reply.
   @return 1 or reply size on success, 0 on failure (error, incoming length does not match, timeout)
*/
int csp_transaction_persistent(csp_conn_t *conn, uint32_t timeout, void *outbuf, int outlen, void *inbuf, int inlen);

/**
   Read data from a connection-less server socket.
   @param[in] socket connection-less socket.
   @param[in] timeout timeout in mS to wait for a packet, use #CSP_MAX_TIMEOUT for infinite timeout.
   @return Packet on success, or NULL on failure or timeout.
*/
csp_packet_t *csp_recvfrom(csp_socket_t *socket, uint32_t timeout);

/**
   Send a packet (without connection).
   @param[in] prio packet priority, see #csp_prio_t
   @param[in] dst destination address
   @param[in] dst_port destination port
   @param[in] src_port source port
   @param[in] opts connection options, see @ref CSP_CONNECTION_OPTIONS.
   @param[in] packet packet to send
   @param[in] timeout unused as of CSP version 1.6
   @return #CSP_ERR_NONE on success, otherwise an error code and the packet must be freed by calling csp_buffer_free().
*/
int csp_sendto(uint8_t prio, uint8_t dst, uint8_t dst_port, uint8_t src_port, uint32_t opts, csp_packet_t *packet, uint32_t timeout);

/**
   Send a packet as a reply to a request (without a connection).
   Calls csp_sendto() with the source address and port from the request.
   @param[in] request incoming request
   @param[in] reply reply packet
   @param[in] opts connection options, see @ref CSP_CONNECTION_OPTIONS.
   @param[in] timeout unused as of CSP version 1.6
   @return #CSP_ERR_NONE on success, otherwise an error code and the reply must be freed by calling csp_buffer_free().
*/
int csp_sendto_reply(const csp_packet_t * request, csp_packet_t * reply, uint32_t opts, uint32_t timeout);

/**
   Establish outgoing connection.
   The call will return immediately, unless it is a RDP connection (#CSP_O_RDP) in which case it will wait until the other
   end acknowleges the connection (timeout is determined by the current connection timeout set by csp_rdp_set_opt()).
   @param[in] prio priority, see #csp_prio_t
   @param[in] dst Destination address
   @param[in] dst_port Destination port
   @param[in] timeout unused.
   @param[in] opts connection options, see @ref CSP_CONNECTION_OPTIONS.
   @return Established connection or NULL on failure (no free connections, timeout).
*/
csp_conn_t *csp_connect(uint8_t prio, uint8_t dst, uint8_t dst_port, uint32_t timeout, uint32_t opts);

/**
   Close an open connection.
   Any packets in the RX queue will be freed.
   @param[in] conn connection. Closing a NULL connection is acceptable.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_close(csp_conn_t *conn);

/**
   Return destination port of connection.
   @param[in] conn connection
   @return destination port of an incoming connection
*/
int csp_conn_dport(csp_conn_t *conn);

/**
   Return source port of connection.
   @param conn connection
   @return source port of an incoming connection
*/
int csp_conn_sport(csp_conn_t *conn);

/**
   Return destination address of connection.
   @param[in] conn connection
   @return destination address of an incoming connection
*/
int csp_conn_dst(csp_conn_t *conn);

/**
   Return source address of connection.
   @param conn connection
   @return source address of an incoming connection
*/
int csp_conn_src(csp_conn_t *conn);

/**
   Return flags of connection.
   @param[in] conn connection
   @return flags of an incoming connection, see @ref CSP_HEADER_FLAGS
*/
int csp_conn_flags(csp_conn_t *conn);

/**
   Set socket to listen for incoming connections.
   @param[in] socket socket
   @param[in] backlog max length of backlog queue. The backlog queue holds incoming connections, waiting to be returned by call to csp_accept().
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_listen(csp_socket_t *socket, size_t backlog);

/**
   Bind port to socket.
   @param[in] socket socket to bind port to
   @param[in] port port number to bind, use #CSP_ANY for all ports. Bindnig to a specific will take precedence over #CSP_ANY.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_bind(csp_socket_t *socket, uint8_t port);

/**
   Start the router task.
   The router task calls csp_route_work() to do the actual work.
   @param[in] task_stack_size stack size for the task, see csp_thread_create() for details on the stack size parameter.
   @param[in] task_priority priority for the task, see csp_thread_create() for details on the stack size parameter.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_route_start_task(unsigned int task_stack_size, unsigned int task_priority);

/**
   Route packet from the incoming router queue and check RDP timeouts.
   In order for incoming packets to routed and RDP timeouts to be checked, this function must be called reguarly.
   If the router task is started by calling csp_route_start_task(), there function should not be called.
   @param[in] timeout timeout in mS to wait for an incoming packet.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_route_work(uint32_t timeout);

/**
   Start the bridge task.
   The bridge will copy packets between interfaces, i.e. packets received on A will be sent on B, and vice versa.
   @param[in] task_stack_size stack size for the task, see csp_thread_create() for details on the stack size parameter.
   @param[in] task_priority priority for the task, see csp_thread_create() for details on the stack size parameter.
   @param[in] if_a interface/side A
   @param[in] if_b interface/side B
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_bridge_start(unsigned int task_stack_size, unsigned int task_priority, csp_iface_t * if_a, csp_iface_t * if_b);

/**
   Handle CSP service request.
   If the given packet is a service-request (the destination port matches one of CSP service ports #csp_service_port_t),
   the packet will be processed by the specific CSP service handler.
   The packet will either process it or free it, so this function is typically called in the last "default" clause of
   a switch/case statement in a CSP listener task.
   In order to listen to csp service ports, bind your listener to the specific services ports #csp_service_port_t or
   use #CSP_ANY to all ports.
   @param[in] conn connection
   @param[in] packet first packet, obtained by using csp_read()
*/
void csp_service_handler(csp_conn_t *conn, csp_packet_t *packet);

/**
   Send a single ping/echo packet.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in ms to wait for reply.
   @param[in] size payload size in bytes.
   @param[in] opts connection options, see @ref CSP_CONNECTION_OPTIONS.
   @return >0 = echo time in mS on success, otherwise -1 for error.
*/
int csp_ping(uint8_t node, uint32_t timeout, unsigned int size, uint8_t opts);

/**
   Send a single ping/echo packet without waiting for reply.
   Payload is 1 byte.
   @param[in] node address of subsystem.
*/
void csp_ping_noreply(uint8_t node);

/**
   Request process list.
   @note This is currently only supported on FreeRTOS systems.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for replies. The function will not return until the timeout occurrs.
*/
void csp_ps(uint8_t node, uint32_t timeout);

/**
   Request free memory.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for reply.
   @param[out] size free memory on subsystem.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_get_memfree(uint8_t node, uint32_t timeout, uint32_t * size);

/**
   Request free memory and print to stdout.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for reply.
*/
void csp_memfree(uint8_t node, uint32_t timeout);

/**
   Request free buffers.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for reply.
   @param[out] size free buffers.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_get_buf_free(uint8_t node, uint32_t timeout, uint32_t * size);

/**
   Request free buffers and print to stdout.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for reply.
*/
void csp_buf_free(uint8_t node, uint32_t timeout);

/**
   Reboot subsystem.
   If handled by the standard CSP service handler, the reboot handler set by csp_sys_set_reboot() on the subsystem, will be invoked.
   @param[in] node address of subsystem.
*/
void csp_reboot(uint8_t node);

/**
   Shutdown subsystem.
   If handled by the standard CSP service handler, the shutdown handler set by csp_sys_set_shutdown() on the subsystem, will be invoked.
   @param[in] node address of subsystem.
*/
void csp_shutdown(uint8_t node);

/**
   Request uptime and print to stdout.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for reply.
*/
void csp_uptime(uint8_t node, uint32_t timeout);

/**
   Request uptime
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for reply.
   @param[out] uptime uptime in seconds.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_get_uptime(uint8_t node, uint32_t timeout, uint32_t * uptime);

/**
   Set RDP options.
   The RDP options are used from the connecting/client side. When a RDP connection is established, the client tranmits the options to the server.
   @param[in] window_size window size
   @param[in] conn_timeout_ms connection timeout in mS
   @param[in] packet_timeout_ms packet timeout in mS.
   @param[in] delayed_acks enable/disable delayed acknowledgements.
   @param[in] ack_timeout acknowledgement timeout when delayed ACKs is enabled
   @param[in] ack_delay_count send acknowledgement for every ack_delay_count packets.
*/
void csp_rdp_set_opt(unsigned int window_size, unsigned int conn_timeout_ms,
		unsigned int packet_timeout_ms, unsigned int delayed_acks,
		unsigned int ack_timeout, unsigned int ack_delay_count);

/**
   Get RDP options.
   @see csp_rdp_set_opt()
   @param[out] window_size Window size
   @param[out] conn_timeout_ms connection timeout in ms
   @param[out] packet_timeout_ms packet timeout in ms
   @param[out] delayed_acks enable/disable delayed acknowledgements
   @param[out] ack_timeout acknowledgement timeout when delayed ACKs is enabled
   @param[out] ack_delay_count send acknowledgement for every ack_delay_count packets
*/
void csp_rdp_get_opt(unsigned int *window_size, unsigned int *conn_timeout_ms,
		unsigned int *packet_timeout_ms, unsigned int *delayed_acks,
		unsigned int *ack_timeout, unsigned int *ack_delay_count);

/**
   Print connection table to stdout.
*/
void csp_conn_print_table(void);

/**
   Print connection table to string.
*/
int csp_conn_print_table_str(char * str_buf, int str_size);

/**
   Print buffer usage table to stdout.
*/
void csp_buffer_print_table(void);

/**
   Hex dump memory to stdout.
   @param[in] desc description printed on first line.
   @param[in] addr memory address.
   @param[in] len number of bytes to dump, starting from \a addr.
*/
void csp_hex_dump(const char *desc, void *addr, int len);

/**
   Set platform specific memory copy function.
*/
void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc);

#ifdef __cplusplus
}
#endif
#endif
