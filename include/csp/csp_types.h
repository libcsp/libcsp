/****************************************************************************
 * **File:** csp/csp_types.h
 *
 * **Description:** Basic types
 ****************************************************************************/
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "csp/autoconfig.h" // -> CSP_X defines (compile configuration)
#include <csp/csp_error.h>
#include <csp/arch/csp_queue.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t tv_sec;
	uint32_t tv_nsec;
} csp_timestamp_t;

/**
 *  Reserved ports for CSP services.
 */
typedef enum {
	CSP_CMP				= 0,   /*< CSP management, e.g. memory, routes, stats */
	CSP_PING			= 1,   /*< Ping - return ping */
	CSP_PS				= 2,   /*< Current process list */
	CSP_MEMFREE			= 3,   /*< Free memory */
	CSP_REBOOT			= 4,   /*< Reboot, see #CSP_REBOOT_MAGIC and #CSP_REBOOT_SHUTDOWN_MAGIC */
	CSP_BUF_FREE		= 5,   /*< Free CSP buffers */
	CSP_UPTIME			= 6,   /*< Uptime */
} csp_service_port_t;

/** Listen on all ports, primarily used with csp_bind() */
#define CSP_ANY				255

/**
   Message priority.
*/
typedef enum {
	CSP_PRIO_CRITICAL		= 0, //!< Critical
	CSP_PRIO_HIGH			= 1, //!< High
	CSP_PRIO_NORM			= 2, //!< Normal (default)
	CSP_PRIO_LOW			= 3, //!< Low
} csp_prio_t;

/**
   CSP identifier/header.
*/
typedef struct  __packed {
	uint8_t pri;
	uint8_t flags;
	uint16_t src;
	uint16_t dst;
	uint8_t dport;
	uint8_t sport;
} csp_id_t ;

/**
   @defgroup CSP_HEADER_FLAGS CSP header flags.
   @{
*/
#define CSP_FRES1			0x80 /*< Reserved for future use */
#define CSP_FRES2			0x40 /*< Reserved for future use */
#define CSP_FRES3			0x20 /*< Reserved for future use */
#define CSP_FFRAG			0x10 /*< Use fragmentation */
#define CSP_FHMAC			0x08 /*< Use HMAC verification */
#define CSP_FRDP			0x02 /*< Use RDP protocol */
#define CSP_FCRC32			0x01 /*< Use CRC32 checksum */
/**@}*/

/**
   @defgroup CSP_SOCKET_OPTIONS CSP Socket options.
   @{
*/
#define CSP_SO_NONE			0x0000 /*< No socket options */
#define CSP_SO_RDPREQ			0x0001 /*< Require RDP */
#define CSP_SO_RDPPROHIB		0x0002 /*< Prohibit RDP */
#define CSP_SO_HMACREQ			0x0004 /*< Require HMAC */
#define CSP_SO_HMACPROHIB		0x0008 /*< Prohibit HMAC */
#define CSP_SO_CRC32REQ			0x0040 /*< Require CRC32 */
#define CSP_SO_CRC32PROHIB		0x0080 /*< Prohibit CRC32 */
#define CSP_SO_CONN_LESS		0x0100 /*< Enable Connection Less mode */
#define CSP_SO_SAME			0x8000 /*< Copy opts from incoming packet only apllies to csp_sendto_reply() */

/**@}*/

/** CSP Connect options */
#define CSP_O_NONE			CSP_SO_NONE        /*< No connection options */
#define CSP_O_RDP			CSP_SO_RDPREQ      /*< Enable RDP */
#define CSP_O_NORDP			CSP_SO_RDPPROHIB   /*< Disable RDP */
#define CSP_O_HMAC			CSP_SO_HMACREQ     /*< Enable HMAC */
#define CSP_O_NOHMAC			CSP_SO_HMACPROHIB  /*< Disable HMAC */
#define CSP_O_CRC32			CSP_SO_CRC32REQ    /*< Enable CRC32 */
#define CSP_O_NOCRC32			CSP_SO_CRC32PROHIB /*< Disable CRC32 */
#define CSP_O_SAME			CSP_SO_SAME

#ifndef CSP_PACKET_PADDING_BYTES
#define CSP_PACKET_PADDING_BYTES 8
#endif

/* This struct is referenced in documentation.  Update doc when you change this. */
/**
 * CSP Packet.
 *
 * This structure is constructed to fit with all interface and protocols to prevent the
 * need to copy data (zero copy).
 *
 * .. note:: In most cases a CSP packet cannot be reused in case of send failure, because the
 * 			 lower layers may add additional data causing increased length (e.g. CRC32), convert
 * 			 the CSP id to different endian (e.g. I2C), etc.
 *
 */
typedef struct csp_packet_s {

	uint32_t timestamp_tx;		/*< Time the message was sent */
	struct csp_conn_s * conn;   /*< Associated connection (this is used in RDP queue) */

	uint16_t rx_count;          /*< Received bytes */
	uint16_t remain;            /*< Remaining packets */
	uint32_t cfpid;             /*< Connection CFP identification number */
	uint32_t last_used;         /*< Timestamp in ms for last use of buffer */
	uint8_t * frame_begin;
	uint16_t frame_length;

	uint16_t length;			/*< Data length */
	csp_id_t id;				/*< CSP id (unpacked version CPU readable) */

	struct csp_packet_s * next; /*< Used for lists / queues of packets */


	/**
	 * Additional header bytes, to prepend packed data before transmission
	 * This must be minimum 6 bytes to accomodate CSP 2.0. But some implementations
	 * require much more scratch working area for encryption for example.
	 *
	 * Ultimately after csp_id_pack() this area will be filled with the CSP header
	 */

	uint8_t header[CSP_PACKET_PADDING_BYTES];

	/**
	 * Data part of packet:
	 */
	union {
		uint8_t data[CSP_BUFFER_SIZE];
		uint16_t data16[CSP_BUFFER_SIZE / 2];
		uint32_t data32[CSP_BUFFER_SIZE / 4];
	};

} csp_packet_t;

#define CSP_RDP_HEADER_SIZE 5

/** Forward declaration of CSP interface, see #csp_iface_s for details. */
typedef struct csp_iface_s csp_iface_t;

typedef void (*csp_callback_t)(csp_packet_t * packet);

/** @brief Connection struct */
struct csp_socket_s {
	csp_queue_handle_t rx_queue;        /*< Queue for RX packets */
	csp_static_queue_t rx_queue_static; /*< Static storage for rx queue */
	char rx_queue_static_data[sizeof(csp_packet_t *) * CSP_CONN_RXQUEUE_LEN];

	uint32_t opts;              /*< Connection or socket options */
};

/** Forward declaration of socket structure */
typedef struct csp_socket_s csp_socket_t;
/** Forward declaration of connection structure */
typedef struct csp_conn_s csp_conn_t;

/** Max length of host name - including zero termination */
#define CSP_HOSTNAME_LEN	20
/** Max length of model name - including zero termination */
#define CSP_MODEL_LEN		30

/** Magic number for reboot request, for service-code #CSP_REBOOT */
#define CSP_REBOOT_MAGIC		0x80078007
/** Magic number for shutdown request, for service-code #CSP_REBOOT */
#define CSP_REBOOT_SHUTDOWN_MAGIC	0xD1E5529A

#ifdef __AVR__
typedef uint32_t csp_memptr_t;
typedef const uint32_t csp_const_memptr_t;
#else
/** Memory pointer */
typedef void * csp_memptr_t;
/** Const memory pointer */
typedef const void * csp_const_memptr_t;
#endif

/**
 * Platform specific memory copy function.
 */
typedef csp_memptr_t (*csp_memcpy_fnc_t)(csp_memptr_t, csp_const_memptr_t, size_t);

/**
 * Compile check/asserts.
 */
#define CSP_STATIC_ASSERT(condition, name)   typedef char name[(condition) ? 1 : -1]

#ifdef __cplusplus
}
#endif
