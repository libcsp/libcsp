/*
 * csp_if_can_pbuf.h
 *
 *  Created on: Feb 3, 2017
 *      Author: johan
 */

#ifndef LIB_CSP_SRC_INTERFACES_CSP_IF_CAN_PBUF_H_
#define LIB_CSP_SRC_INTERFACES_CSP_IF_CAN_PBUF_H_

/* Packet buffers */
typedef enum {
	BUF_FREE = 0,			/* Buffer element free */
	BUF_USED = 1,			/* Buffer element used */
} csp_can_pbuf_state_t;

typedef struct {
	uint16_t rx_count;		/* Received bytes */
	uint32_t remain;		/* Remaining packets */
	uint32_t cfpid;			/* Connection CFP identification number */
	csp_packet_t *packet;		/* Pointer to packet buffer */
	csp_can_pbuf_state_t state;	/* Element state */
	uint32_t last_used;		/* Timestamp in ms for last use of buffer */
} csp_can_pbuf_element_t;

int csp_can_pbuf_free(csp_can_pbuf_element_t *buf, CSP_BASE_TYPE *task_woken);
csp_can_pbuf_element_t *csp_can_pbuf_new(uint32_t id, CSP_BASE_TYPE *task_woken);
csp_can_pbuf_element_t *csp_can_pbuf_find(uint32_t id, uint32_t mask);
void csp_can_pbuf_cleanup(CSP_BASE_TYPE *task_woken);

#endif /* LIB_CSP_SRC_INTERFACES_CSP_IF_CAN_PBUF_H_ */
