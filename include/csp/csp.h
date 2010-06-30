/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 Gomspace ApS (gomspace.com)
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

#ifndef _CSP_H_
#define _CSP_H_

/* Includes */
#include <stdint.h>
#include <stdlib.h>

/* CSP includes */
#include "csp_platform.h"
#include "csp_config.h"

/**
 * RESERVED PORTS (SERVICES)
 */

#define CSP_ANY             16
#define CSP_PING            1
#define CSP_PS              2
#define CSP_MEMFREE         3
#define CSP_REBOOT          4
#define CSP_BUF_FREE        5

/**
 * PRIORITIES
 */

#define CSP_PRIO_CRITICAL   0
#define CSP_PRIO_ALERT      1
#define CSP_PRIO_HIGH       2
#define CSP_PRIO_RESERVED   3
#define CSP_PRIO_NORM       4
#define CSP_PRIO_LOW        5
#define CSP_PRIO_BULK       6
#define CSP_PRIO_DEBUG      7

/**
 * CSP FRAME TYPES
 */

#define CSP_RESERVED1       0
#define CSP_RESERVED2       1
#define CSP_BEGIN           2
#define CSP_ACK             3
#define CSP_ERROR           4
#define CSP_MORE            5
#define CSP_RESERVED3       6
#define CSP_RESERVED4       7

/** @brief The address of the node */
extern uint8_t my_address;

/** Identifier field masks */
#define CSP_ID_PRIO_MASK    ((uint32_t)0x07 << 26)
#define CSP_ID_SRC_MASK     ((uint32_t)0x0F << 22)
#define CSP_ID_DST_MASK     ((uint32_t)0x0F << 18)
#define CSP_ID_DPORT_MASK   ((uint32_t)0x1F << 13)
#define CSP_ID_SPORT_MASK   ((uint32_t)0x1F << 8)
#define CSP_ID_TYPE_MASK    ((uint32_t)0x07 << 5)
#define CSP_ID_SEQ_MASK     ((uint32_t)0x1F << 0)
#define CSP_ID_MASK         (CSP_ID_PRIO_MASK  | \
                             CSP_ID_SRC_MASK   | \
                             CSP_ID_DST_MASK   | \
                             CSP_ID_DPORT_MASK | \
                             CSP_ID_SPORT_MASK | \
                             CSP_ID_TYPE_MASK  | \
                             CSP_ID_SEQ_MASK)
#define CSP_ID_CONN_MASK    (CSP_ID_SRC_MASK | CSP_ID_DST_MASK | CSP_ID_DPORT_MASK | CSP_ID_SPORT_MASK)

/** @brief This union defines a CSP identifier and allows to access it in mode standard, extended or through a table. */
typedef union {
  uint32_t ext;
  uint16_t std;
  uint8_t  tab[4];
  struct __attribute__((__packed__)) {

#if defined(_CSP_BIG_ENDIAN_) && !defined(_CSP_LITTLE_ENDIAN_)

    unsigned int res : 3;
    unsigned int pri : 3;
    unsigned int src : 4;
    unsigned int dst : 4;
    unsigned int dport : 5;
    unsigned int sport : 5;
    unsigned int type : 3;
    unsigned int seq : 5;

#elif defined(_CSP_LITTLE_ENDIAN_) && !defined(_CSP_BIG_ENDIAN_)

    unsigned int seq : 5;
    unsigned int type : 3;
    unsigned int sport : 5;
    unsigned int dport : 5;
    unsigned int dst : 4;
    unsigned int src : 4;
    unsigned int pri : 3;
    unsigned int res : 3;

#else

  #error "Must define one of _CSP_BIG_ENDIAN_ or _CSP_LITTLE_ENDIAN_ in csp_platform.h"

#endif

  };
} csp_id_t;

/**
 * CSP PACKET STRUCTURE
 * Note: This structure is constructed to fit
 * with all interface frame types in order to 
 * have buffer reuse
 */
typedef struct __attribute__((__packed__)) {
    uint8_t padding[44];       // Interface dependent padding
    uint16_t length;            // Length field must be just before CSP ID
    csp_id_t id;                // CSP id must be just before data
    uint8_t data[];				// This just points to the rest of the buffer, without a size indication.
} csp_packet_t;

/**
 * This define must equial the size of the packet overhead in csp_packet_t
 * it is used in csp_buffer_get() to check the allocated buffer size against
 * thre required buffer size.
 */
#define CSP_BUFFER_PACKET_OVERHEAD 	44+2+4

typedef struct csp_socket_s csp_socket_t;
typedef struct csp_conn_s csp_conn_t;

/* Implemented in csp_io.c */
void csp_init(uint8_t my_node_address);
csp_socket_t * csp_socket();
csp_conn_t * csp_accept(csp_socket_t * socket, int timeout);
csp_packet_t * csp_read(csp_conn_t * conn, int timeout);
int csp_send(csp_conn_t * conn, csp_packet_t * packet, int timeout);
int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, int timeout, void * outbuf, int outlen, void * inbuf, int inlen);
int csp_transaction_persistent(csp_conn_t * conn, int timeout, void * outbuf, int outlen, void * inbuf, int inlen);

/* Implemented in csp_conn.c */
csp_conn_t * csp_connect(uint8_t prio, uint8_t dest, uint8_t port);
void csp_close(csp_conn_t * conn);
int csp_conn_dport(csp_conn_t * conn);
int csp_conn_sport(csp_conn_t * conn);
int csp_conn_dst(csp_conn_t * conn);
int csp_conn_src(csp_conn_t * conn);

/* Implemented in csp_port.c */
int csp_listen(csp_socket_t * socket, size_t conn_queue_length);
int csp_bind_callback(void (*callback) (csp_conn_t*), uint8_t port);
int csp_bind(csp_socket_t * socket, uint8_t port);

/* Implemented in csp_route.c */
typedef int (*nexthop_t)(csp_id_t idout, csp_packet_t * packet, unsigned int timeout);
void csp_route_set(const char * name, uint8_t node, nexthop_t nexthop);
void csp_new_packet(csp_packet_t * packet, nexthop_t interface, CSP_BASE_TYPE * pxTaskWoken);
void csp_route_start_task(unsigned int task_stack_size);

/* Implemented in csp_services.c */
void csp_service_handler(csp_conn_t * conn, csp_packet_t * packet);
void csp_ping(uint8_t node, int timeout);
void csp_ping_noreply(uint8_t node);
void csp_ps(uint8_t node, int timeout);
void csp_memfree(uint8_t node, int timeout);
void csp_buf_free(uint8_t node, int timeout);
void csp_reboot(uint8_t node);

/**
 * Start the buffer handling system
 * You must specify the number for buffers and the size. All buffers are fixed
 * size so you must specify the size of your largest buffer. The standard for
 * GomSpace network library is 310 bytes.
 *
 * @param count: Number of buffers to allocate
 * @param size: Buffer size in bytes.
 *
 * @return 0 if malloc() failed, 1 if sucessful.
 */
int csp_buffer_init(int count, int size);

/**
 * Get a reference to a free buffer. This call is both interrupt and thread
 * safe. This function is similar to malloc()
 *
 * @param size: Specify what data-size you will put in the buffer
 * @return NULL if size is larger than buffer-block-sizse, pointer otherwise.
 */
void * csp_buffer_get(size_t size);

/**
 * Free a buffer after use. This call is both interrupt and thread safe.
 * This function is similar to free()
 *
 * @param pointer to memory area, must be aquired by csp_buffer_get().
 */
void csp_buffer_free(void * packet);

/**
 * Return how many buffers that are currently free.
 * @return number of free buffers
 */
int csp_buffer_remaining(void);

/* CSP debug printf - implemented in arch/x/csp_debug.c */
void csp_debug(const char * format, ...);

#endif // _CSP_H_
