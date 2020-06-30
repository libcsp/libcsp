/*
 * csp_id.c
 *
 *  Created on: 30. jun. 2020
 *      Author: johan
 */

#include <csp/csp.h>

#define CSP_ID1_PRIO_SIZE		2
#define CSP_ID1_HOST_SIZE		5
#define CSP_ID1_PORT_SIZE		6
#define CSP_ID1_FLAGS_SIZE		8

typedef struct __attribute__((__packed__)) {
#if (CSP_BIG_ENDIAN || __DOXYGEN__)
        unsigned int pri   : CSP_ID1_PRIO_SIZE;
        unsigned int src   : CSP_ID1_HOST_SIZE;
        unsigned int dst   : CSP_ID1_HOST_SIZE;
        unsigned int dport : CSP_ID1_PORT_SIZE;
        unsigned int sport : CSP_ID1_PORT_SIZE;
        unsigned int flags : CSP_ID1_FLAGS_SIZE;
#elif (CSP_LITTLE_ENDIAN)
        unsigned int flags : CSP_ID1_FLAGS_SIZE;
        unsigned int sport : CSP_ID1_PORT_SIZE;
        unsigned int dport : CSP_ID1_PORT_SIZE;
        unsigned int dst   : CSP_ID1_HOST_SIZE;
        unsigned int src   : CSP_ID1_HOST_SIZE;
        unsigned int pri   : CSP_ID1_PRIO_SIZE;
#endif
} csp_id1_t;

#define CSP_ID2_PRIO_SIZE		2
#define CSP_ID2_HOST_SIZE		14
#define CSP_ID2_PORT_SIZE		6
#define CSP_ID2_FLAGS_SIZE		6

typedef struct __attribute__((__packed__)) {
#if (CSP_BIG_ENDIAN || __DOXYGEN__)
        unsigned int pri   : CSP_ID2_PRIO_SIZE;
        unsigned int src   : CSP_ID2_HOST_SIZE;
        unsigned int dst   : CSP_ID2_HOST_SIZE;
        unsigned int dport : CSP_ID2_PORT_SIZE;
        unsigned int sport : CSP_ID2_PORT_SIZE;
        unsigned int flags : CSP_ID2_FLAGS_SIZE;
#elif (CSP_LITTLE_ENDIAN)
        unsigned int flags : CSP_ID2_FLAGS_SIZE;
        unsigned int sport : CSP_ID2_PORT_SIZE;
        unsigned int dport : CSP_ID2_PORT_SIZE;
        unsigned int dst   : CSP_ID2_HOST_SIZE;
        unsigned int src   : CSP_ID2_HOST_SIZE;
        unsigned int pri   : CSP_ID2_PRIO_SIZE;
#endif
} csp_id2_t;

void csp_id1_prepend(csp_packet_t * packet) {

	csp_hex_dump("Before prepend 1", packet, 64);

	csp_id1_t id1;
	id1.pri = packet->id.pri;
	id1.dst = packet->id.dst;
	id1.src = packet->id.src;
	id1.dport = packet->id.dport;
	id1.sport = packet->id.sport;
	id1.flags = packet->id.flags;

	packet->frame_begin = packet->data - sizeof(csp_id1_t);
	packet->frame_length = packet->length + sizeof(csp_id1_t);

	memcpy(packet->frame_begin, &id1, sizeof(csp_id1_t));

	csp_hex_dump("After prepend 1 ", packet, 64);

}

int csp_id1_strip(csp_packet_t * packet) {

	if (packet->frame_length < sizeof(csp_id2_t)) {
		printf("Too short frame\n");
		return -1;
	}

	printf("Strip 1 %p %p\n", packet, packet->frame_begin);

	csp_id1_t id1;
	memcpy(&id1, packet->frame_begin, sizeof(csp_id1_t));
	packet->length = packet->frame_length - sizeof(csp_id1_t);
	packet->id.pri = id1.pri;
	packet->id.dst = id1.dst;
	packet->id.src = id1.src;
	packet->id.dport = id1.dport;
	packet->id.sport = id1.sport;
	packet->id.flags = id1.flags;

	return 0;

}

void csp_id1_setup_rx(csp_packet_t * packet) {
	packet->frame_begin = packet->data - sizeof(csp_id1_t);
	packet->frame_length = 0;
}

void csp_id2_prepend(csp_packet_t * packet) {

	csp_hex_dump("Before prepend 2", packet, 64);
	printf("Sizeof id2 %lu\n", sizeof(csp_id2_t));

	csp_id2_t id2;
	id2.pri = packet->id.pri;
	id2.dst = packet->id.dst;
	id2.src = packet->id.src;
	id2.dport = packet->id.dport;
	id2.sport = packet->id.sport;
	id2.flags = packet->id.flags;

	packet->frame_begin = packet->data - sizeof(csp_id2_t);
	packet->frame_length = packet->length + sizeof(csp_id2_t);

	memcpy(packet->frame_begin, &id2, sizeof(csp_id2_t));

	csp_hex_dump("After prepend 2", packet, 64);

}

int csp_id2_strip(csp_packet_t * packet) {

	if (packet->frame_length < sizeof(csp_id2_t)) {
		printf("Too short frame\n");
		return -1;
	}

	printf("Strip 2 %p %p\n", packet, packet->frame_begin);

	csp_id2_t id2;
	memcpy(&id2, packet->frame_begin, sizeof(csp_id2_t));
	packet->length = packet->frame_length - sizeof(csp_id2_t);
	packet->id.pri = id2.pri;
	packet->id.dst = id2.dst;
	packet->id.src = id2.src;
	packet->id.dport = id2.dport;
	packet->id.sport = id2.sport;
	packet->id.flags = id2.flags;

	return 0;

}

void csp_id2_setup_rx(csp_packet_t * packet) {
	packet->frame_begin = packet->data - sizeof(csp_id2_t);
	packet->frame_length = 0;
}

void csp_id_prepend(csp_packet_t * packet) {
	csp_id2_prepend(packet);
}

int csp_id_strip(csp_packet_t * packet) {
	return csp_id2_strip(packet);
}

void csp_id_setup_rx(csp_packet_t * packet) {
	csp_id2_setup_rx(packet);
}
