/** @file csp.h
 * @brief CSP: Cubesat Space Protocol
 *
 * Stream oriented transport layer protocol for small cubesat networks up to 16 nodes.
 *
 * @author 2006-2009 Johan Christiansen, Aalborg University, Denmark
 * @author 2009 Gomspace A/S
 *
 * @version 0.9 (Pre-release)
 *
 * @todo Implement message priorities in router
 *
 */

#ifndef _CSP_H_
#define _CSP_H_

/* Includes */
#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <stdint.h>

#define MAX_STATIC_CONNS 2 /**< Number of staticly allocated connection structs */
#define CSP_MTU 260+4

/**
 * RESERVED PORTS (SERVICES)
 */

#define		CSP_ANY				16
#define		CSP_PING            1
#define		CSP_PS				2
#define		CSP_MEMFREE			3
#define		CSP_REBOOT			4
#define		CSP_BUF_FREE		5

/**
 * PRIORITIES
 */

#define		PRIO_CRITICAL		0
#define		PRIO_ALERT			1
#define		PRIO_HIGH			2
#define		PRIO_RESERVED		3
#define		PRIO_NORM			4
#define		PRIO_LOW			5
#define		PRIO_BULK			6
#define		PRIO_DEBUG			7


/** @brief The address of the node */
extern unsigned char my_address;

/** @brief This union defines a CSP identifier and allows to access it in mode standard, extended or through a table. */
typedef union{
  uint32_t ext;
  uint16_t std;
  uint8_t  tab[4];
  struct {

#if defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN)

	unsigned int res : 3;
	unsigned int pri : 3;
	unsigned int src : 4;
	unsigned int dst : 4;
	unsigned int dport : 5;
	unsigned int sport : 5;
	unsigned int type : 3;
	unsigned int seq : 5;

#elif defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)

	unsigned int seq : 5;
	unsigned int type : 3;
	unsigned int sport : 5;
	unsigned int dport : 5;
	unsigned int dst : 4;
	unsigned int src : 4;
	unsigned int pri : 3;
	unsigned int res : 3;

#else

  #error "Must define one of BIG_ENDIAN or LITTLE_ENDIAN"

#endif

  };
} csp_id_t;

/**
 * CSP PACKET STRUCTURE
 * Note: This structure is constructed to fit
 * with i2c_frame_t in order to have buffer reuse
 */
typedef struct __attribute__((packed)) {
	uint8_t padding1[42];
	uint8_t	mac_dest;
	uint8_t padding2;
	uint16_t length;					// Length field must be just before CSP ID
	union {
		struct {
			csp_id_t id;				// CSP id must be just before data
			uint8_t data[CSP_MTU-4];	// Data must be large enough to fit a en encoded spacelink frame
		};
		uint8_t packet[CSP_MTU];
	};
} csp_packet_t;

/** @brief Connection states */
typedef enum {
	SOCKET_CLOSED, /**< Connection closed */
	SOCKET_OPEN, /**< Connection open (ready to send) */
} conn_state_t;

/** @brief Connection struct */
typedef struct {
	conn_state_t state;						// Connection state (SOCKET_OPEN or SOCKET_CLOSED)
	csp_id_t idin;							// Identifier received
	csp_id_t idout;							// Idenfifier transmitted
	csp_packet_t * rxmalloc;				// Pointer to the malloc'ed area during reception
	xQueueHandle rxQueue;					// Queue for RX packets
    xSemaphoreHandle xSemaphoretx;			// Binary semaphore for csp_send()
} conn_t;

/** @brief Port states */
typedef enum {
	PORT_CLOSED = 0,
	PORT_OPEN = 1,} port_state_t;

/** @brief Port struct */
typedef struct {
	port_state_t state; 					// Port state
	void (*callback) (conn_t*);				// Pointer to callback function
	xQueueHandle connQueue;					// New connections is added to this queue
} port_t;

/* Implemented in csp_io.c */
void csp_init(unsigned char my_node_address);
conn_t * csp_listen(xQueueHandle connQueue, int timeout);
csp_packet_t * csp_read(conn_t *conn, int timeout);
int csp_send_direct(csp_id_t idout, csp_packet_t * packet, int timeout);
int csp_send(conn_t* conn, csp_packet_t * packet, int timeout);
int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, int timeout, void * outbuf, int outlen, void * inbuf, int inlen);

/* Implemented in csp_conn.c */
void csp_conn_init(void);
conn_t * csp_conn_find(uint32_t id, uint32_t mask);
conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout);
conn_t* csp_connect(uint8_t prio, uint8_t dest, uint8_t port);
void csp_conn_close(conn_t* conn);

/* Implemented in csp_buffer.c */
int csp_buffer_init(int count, int size);
void * csp_buffer_get(size_t size);
void csp_buffer_free(void * packet);
int csp_buffer_remaining(void);

/* Implemented in csp_port.c */
extern port_t ports[];
void csp_port_init(void);
xQueueHandle csp_port_listener(int conn_queue_length);
void csp_port_callback(unsigned char port_nr, void (*callback) (conn_t*));
void csp_port_bind(unsigned char port_nr, xQueueHandle listener);

/* Implemented in csp_route.c */
typedef int (*nexthop_t)(csp_id_t idout, csp_packet_t * packet, unsigned int timeout);
typedef struct {
	const char * name;
	nexthop_t nexthop;
	int count;
} csp_iface_t;
extern csp_iface_t iface[];
void csp_route_set(const char * name, unsigned char node, nexthop_t nexthop);
void csp_route_print(void);
void csp_route_table_init(void);
csp_iface_t * csp_route_if(int id);
conn_t * csp_route(csp_id_t id, nexthop_t interface, portBASE_TYPE * pxTaskWoken);
void csp_set_connection_fallback(xQueueHandle connQueue);
void csp_new_packet(csp_packet_t * packet, nexthop_t interface, portBASE_TYPE * pxTaskWoken);
void vTaskCSPRouter(void * pvParameters);

/* Implemented in csp_services.c */
void csp_service_handler(conn_t * conn, csp_packet_t * packet);
void csp_ping(unsigned char node, portTickType timeout);
void csp_ping_noreply(unsigned char node);
void csp_ps(uint8_t node, portTickType timeout);
void csp_memfree(uint8_t node, portTickType timeout);
void csp_buf_free(uint8_t node, portTickType timeout);
void csp_reboot(uint8_t node);

/* Implemented in csp_console.c */
void csp_console_init(void);

/* Implemented in csp_fc_chunk.c */
int csp_fc_chunk_recv(conn_t * conn, uint8_t * buf, uint32_t offset, uint32_t size);

#endif // _CSP_H_
