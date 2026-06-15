/* CSP Management Protocol. */
#pragma once

#include <csp/csp.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CMP message type. */
#define CSP_CMP_REQUEST 0x00
#define CSP_CMP_REPLY   0xff

/* CMP request codes. */
#define CSP_CMP_IDENT        1
#define CSP_CMP_ROUTE_SET_V1 2
#define CSP_CMP_IF_STATS     3
#define CSP_CMP_PEEK         4
#define CSP_CMP_POKE         5
#define CSP_CMP_CLOCK        6
#define CSP_CMP_ROUTE_SET_V2 7
#define CSP_CMP_PEEK_V2      8
#define CSP_CMP_POKE_V2      9

/* CMP field limits. */
#define CSP_CMP_IDENT_REV_LEN  20
#define CSP_CMP_IDENT_DATE_LEN 12
#define CSP_CMP_IDENT_TIME_LEN 9

#define CSP_CMP_ROUTE_IFACE_LEN 11

#define CSP_CMP_PEEK_MAX_LEN    200
#define CSP_CMP_POKE_MAX_LEN    200
#define CSP_CMP_PEEK_V2_MAX_LEN 196
#define CSP_CMP_POKE_V2_MAX_LEN 196

/*
 * All CMP wire messages start with type and code. The dispatcher validates this
 * header before casting packet data to one of the concrete packed layouts below.
 */
struct csp_cmp_header {
	uint8_t type;
	uint8_t code;
} __attribute__((__packed__));

/* Packed CMP wire messages. */
struct csp_cmp_ident_msg {
	uint8_t type;
	uint8_t code;
	char hostname[CSP_HOSTNAME_LEN];
	char model[CSP_MODEL_LEN];
	char revision[CSP_CMP_IDENT_REV_LEN];
	char date[CSP_CMP_IDENT_DATE_LEN];
	char time[CSP_CMP_IDENT_TIME_LEN];
} __attribute__((__packed__));

struct csp_cmp_route_set_v1_msg {
	uint8_t type;
	uint8_t code;
	uint8_t dest_node;
	uint8_t next_hop_via;
	char interface[CSP_CMP_ROUTE_IFACE_LEN];
} __attribute__((__packed__));

struct csp_cmp_route_set_v2_msg {
	uint8_t type;
	uint8_t code;
	uint16_t dest_node;
	uint16_t next_hop_via;
	uint16_t netmask;
	char interface[CSP_CMP_ROUTE_IFACE_LEN];
} __attribute__((__packed__));

struct csp_cmp_if_stats_msg {
	uint8_t type;
	uint8_t code;
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
} __attribute__((__packed__));

struct csp_cmp_peek_msg {
	uint8_t type;
	uint8_t code;
	uint32_t addr;
	uint8_t len;
	uint8_t data[];
} __attribute__((__packed__));

struct csp_cmp_poke_msg {
	uint8_t type;
	uint8_t code;
	uint32_t addr;
	uint8_t len;
	uint8_t data[];
} __attribute__((__packed__));

struct csp_cmp_peek_v2_msg {
	uint8_t type;
	uint8_t code;
	uint64_t vaddr;
	uint8_t len;
	uint8_t data[];
} __attribute__((__packed__));

struct csp_cmp_poke_v2_msg {
	uint8_t type;
	uint8_t code;
	uint64_t vaddr;
	uint8_t len;
	uint8_t data[];
} __attribute__((__packed__));

struct csp_cmp_clock_msg {
	uint8_t type;
	uint8_t code;
	csp_timestamp_t clock;
} __attribute__((__packed__));

/*
 * Macro for calculating variable-size management messages.
 * Legacy variable CMP messages include the tail padding from the original
 * fixed-size member, but the data bytes themselves are not rounded up.
 */
#define CMP_ALIGN_UP_32(_len) (((_len) + 3u) & ~3u)
#define CMP_VARIABLE_PAYLOAD_SIZE(_packet) (sizeof(struct _packet) - sizeof(struct csp_cmp_header))
#define CMP_VARIABLE_TAIL_SIZE(_packet) (CMP_ALIGN_UP_32(CMP_VARIABLE_PAYLOAD_SIZE(_packet)) - CMP_VARIABLE_PAYLOAD_SIZE(_packet))
#define CMP_VARIABLE_SIZE(_packet, _len) (sizeof(struct _packet) + CMP_VARIABLE_TAIL_SIZE(_packet) + (_len))
#define CMP_PEEK_SIZE(_len) CMP_VARIABLE_SIZE(csp_cmp_peek_msg, _len)
#define CMP_POKE_SIZE(_len) CMP_VARIABLE_SIZE(csp_cmp_poke_msg, _len)
#define CMP_PEEK_V2_SIZE(_len) CMP_VARIABLE_SIZE(csp_cmp_peek_v2_msg, _len)
#define CMP_POKE_V2_SIZE(_len) CMP_VARIABLE_SIZE(csp_cmp_poke_v2_msg, _len)

/* Sets the common CMP header and transacts an exact-size request/reply. */
int csp_cmp(uint16_t node, uint32_t timeout, uint8_t code, int msg_size, void *msg);

static inline int csp_cmp_ident(uint16_t node, uint32_t timeout, struct csp_cmp_ident_msg *msg) {
	return csp_cmp(node, timeout, CSP_CMP_IDENT, sizeof(struct csp_cmp_ident_msg), msg);
}

static inline int csp_cmp_route_set_v1(uint16_t node, uint32_t timeout, struct csp_cmp_route_set_v1_msg *msg) {
	return csp_cmp(node, timeout, CSP_CMP_ROUTE_SET_V1, sizeof(struct csp_cmp_route_set_v1_msg), msg);
}

static inline int csp_cmp_if_stats(uint16_t node, uint32_t timeout, struct csp_cmp_if_stats_msg *msg) {
	return csp_cmp(node, timeout, CSP_CMP_IF_STATS, sizeof(struct csp_cmp_if_stats_msg), msg);
}

static inline int csp_cmp_clock(uint16_t node, uint32_t timeout, struct csp_cmp_clock_msg *msg) {
	return csp_cmp(node, timeout, CSP_CMP_CLOCK, sizeof(struct csp_cmp_clock_msg), msg);
}

static inline int csp_cmp_route_set_v2(uint16_t node, uint32_t timeout, struct csp_cmp_route_set_v2_msg *msg) {
	return csp_cmp(node, timeout, CSP_CMP_ROUTE_SET_V2, sizeof(struct csp_cmp_route_set_v2_msg), msg);
}

static inline int csp_cmp_peek(uint16_t node, uint32_t timeout, struct csp_cmp_peek_msg *msg) {
	return csp_cmp(node, timeout, CSP_CMP_PEEK, CMP_PEEK_SIZE(msg->len), msg);
}

static inline int csp_cmp_poke(uint16_t node, uint32_t timeout, struct csp_cmp_poke_msg *msg) {
	return csp_cmp(node, timeout, CSP_CMP_POKE, CMP_POKE_SIZE(msg->len), msg);
}

static inline int csp_cmp_peek_v2(uint16_t node, uint32_t timeout, struct csp_cmp_peek_v2_msg *msg) {
	return csp_cmp(node, timeout, CSP_CMP_PEEK_V2, CMP_PEEK_V2_SIZE(msg->len), msg);
}

static inline int csp_cmp_poke_v2(uint16_t node, uint32_t timeout, struct csp_cmp_poke_v2_msg *msg) {
	return csp_cmp(node, timeout, CSP_CMP_POKE_V2, CMP_POKE_V2_SIZE(msg->len), msg);
}

#ifdef __cplusplus
}
#endif
