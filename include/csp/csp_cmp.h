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

#ifndef _CSP_CMP_H_
#define _CSP_CMP_H_

/**
   @file
   CSP Management Protocol (CMP).
*/

#include <csp/csp.h>
#include <csp/arch/csp_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   CMP type.
   @{
*/
/**
   CMP request.
*/
#define CSP_CMP_REQUEST 0x00
/**
   CMP reply.
*/
#define CSP_CMP_REPLY   0xff
/**@}*/

/**
   CMP requests.
   @{
*/
/**
   CMP request codes.
*/
/**
   Request identification, compile time, revision, hostname and model.
*/
#define CSP_CMP_IDENT 1
/**
   Set/configure routing.
*/
#define CSP_CMP_ROUTE_SET 2
/**
   Request interface statistics.
*/
#define CSP_CMP_IF_STATS 3
/**
   Peek/read data from memory.
*/
#define CSP_CMP_PEEK 4
/**
   Poke/write data from memory.
*/
#define CSP_CMP_POKE 5
/**
   Get/set clock.
*/
#define CSP_CMP_CLOCK 6
/**@}*/

/**
   CMP identification - max revision length.
*/
#define CSP_CMP_IDENT_REV_LEN  20
/**
   CMP identification - max date length.
*/
#define CSP_CMP_IDENT_DATE_LEN 12
/**
   CMP identification - max time length.
*/
#define CSP_CMP_IDENT_TIME_LEN 9

/**
   CMP interface statistics - max interface name length.
*/
#define CSP_CMP_ROUTE_IFACE_LEN 11

/**
   CMP peek/read memeory - max read length.
*/
#define CSP_CMP_PEEK_MAX_LEN 200

/**
   CMP poke/write memeory - max write length.
*/
#define CSP_CMP_POKE_MAX_LEN 200

/**
   CSP management protocol description.
*/
struct csp_cmp_message {
        //! CMP request type.
        uint8_t type;
        //! CMP request code.
        uint8_t code;
	union {
		struct {
			char hostname[CSP_HOSTNAME_LEN];
			char model[CSP_MODEL_LEN];
			char revision[CSP_CMP_IDENT_REV_LEN];
			char date[CSP_CMP_IDENT_DATE_LEN];
			char time[CSP_CMP_IDENT_TIME_LEN];
		} ident;
		struct {
			uint8_t dest_node;
			uint8_t next_hop_via;
			char interface[CSP_CMP_ROUTE_IFACE_LEN];
		} route_set;
		struct __attribute__((__packed__)) {
			char interface[CSP_CMP_ROUTE_IFACE_LEN];
			uint32_t tx;
			uint32_t rx;
			uint32_t tx_error;
			uint32_t rx_error;
			uint32_t drop;
			uint32_t autherr;
			uint32_t frame;
			uint32_t txbytes;
			uint32_t rxbytes;
			uint32_t irq;
		} if_stats;
		struct {
			uint32_t addr;
			uint8_t len;
			char data[CSP_CMP_PEEK_MAX_LEN];
		} peek;
		struct {
			uint32_t addr;
			uint8_t len;
			char data[CSP_CMP_POKE_MAX_LEN];
		} poke;
		csp_timestamp_t clock;
	};
} __attribute__ ((packed));

/**
   Macro for calculating total size of management message.
*/
#define CMP_SIZE(_memb) (sizeof(((struct csp_cmp_message *)0)->type) + sizeof(((struct csp_cmp_message *)0)->code) + sizeof(((struct csp_cmp_message *)0)->_memb))

/**
   Generic send management message request.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for reply..
   @param[in] code request code.
   @param[in] msg_size size of \a msg.
   @param[in,out] msg data.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_cmp(uint8_t node, uint32_t timeout, uint8_t code, int msg_size, struct csp_cmp_message *msg);

/**
   Macro for defining management handling function.
*/
#define CMP_MESSAGE(_code, _memb) \
static inline int csp_cmp_##_memb(uint8_t node, uint32_t timeout, struct csp_cmp_message *msg) { \
	return csp_cmp(node, timeout, _code, CMP_SIZE(_memb), msg); \
}

CMP_MESSAGE(CSP_CMP_IDENT, ident)
CMP_MESSAGE(CSP_CMP_ROUTE_SET, route_set)
CMP_MESSAGE(CSP_CMP_IF_STATS, if_stats)
CMP_MESSAGE(CSP_CMP_CLOCK, clock)

/**
   Peek (read) memory on remote node.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for reply..
   @param[in] msg memory address and number of bytes to peek.
   @param[out] msg peeked/read memory.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
static inline int csp_cmp_peek(uint8_t node, uint32_t timeout, struct csp_cmp_message *msg) {
    return csp_cmp(node, timeout, CSP_CMP_PEEK, CMP_SIZE(peek) - sizeof(msg->peek.data) + msg->peek.len, msg);
}

/**
   Poke (write) memory on remote node.
   @param[in] node address of subsystem.
   @param[in] timeout timeout in mS to wait for reply..
   @param[in] msg memory address, number of bytes and the actual bytes to poke/write.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
static inline int csp_cmp_poke(uint8_t node, uint32_t timeout, struct csp_cmp_message *msg) {
    return csp_cmp(node, timeout, CSP_CMP_POKE, CMP_SIZE(poke) - sizeof(msg->poke.data) + msg->poke.len, msg);
}

#ifdef __cplusplus
}
#endif
#endif
