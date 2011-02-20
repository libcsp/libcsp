/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
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

/* AT91SAM7A3 driver */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atmel/AT91SAM7A3.h>
#include <atmel/aic.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

#include "can.h"
#include "can_at91sam7a3.h"

/* MOB segmentation */
#define CAN_RX_MBOX 8
#define CAN_TX_MBOX 8
#define CAN_MBOXES 	16

/** Callback functions */
can_tx_callback_t txcb = NULL;
can_rx_callback_t rxcb = NULL;

/** Identifier and mask */
uint32_t can_id;
uint32_t can_mask;

/** Mailbox */
typedef enum {
    MBOX_FREE = 0,
    MBOX_USED = 1,
} mbox_t;

/** List of mobs */
static mbox_t mbox[CAN_TX_MBOX];

/** Calculate mode register. Each bit is divided in 10 time quanta
 *  Fixed values: PROPAG=0 SJW=1 SMP=1 PHASE1=3 PHASE2=3 */
#define CAN_MODE(bitrate,clock_speed) (0x01001033 | (((clock_speed)/(10 * (bitrate)) - 1) << 16))

/** Pointers to the hardware */
can_controller_t * const CAN_CTRL = ((can_controller_t *)CAN0_BASE_ADDRESS);

/** ISR prototype */
static void can_isr(void);

/** Setup CAN interrupts */
static void can_init_interrupt(uint32_t id, uint32_t mask) {
	uint8_t mbox;

	/* Configure ISR */
	AIC_ConfigureIT(AT91C_ID_CAN0, AT91C_AIC_PRIOR_HIGHEST, (void(*)(void)) can_isr);

	/* Enable interrupts for all mailboxes */
	for (mbox = 0; mbox < CAN_MBOXES; mbox++) {
		if (mbox >= CAN_TX_MBOX) {
			/* Setup the mask and ID */
			CAN_CTRL->CHANNEL[mbox].MAM = (mask & 0x1FFFFFFF) | MIDE;
			CAN_CTRL->CHANNEL[mbox].MID = (id   & 0x1FFFFFFF) | MIDE;

			/* Allow next transmission */
			CAN_CTRL->CHANNEL[mbox].MCR = MTCR;

			/* Configure mailbox as Rx */
			CAN_CTRL->CHANNEL[mbox].MMR = (0x01 << 24);
		} else {
			/* Configure mailbox as Tx */
			CAN_CTRL->CHANNEL[mbox].MMR = (0x03 << 24);
		}

		/* Enable interrupts for mailbox */
		CAN_CTRL->IER = (1 << mbox);
	}

	/* Enable interrupt */
	AIC_EnableIT(AT91C_ID_CAN0);

}

int can_init(uint32_t id, uint32_t mask, can_tx_callback_t atxcb, can_rx_callback_t arxcb, void * conf, int conflen) {

	uint32_t bitrate;
	uint32_t clock_speed;
	struct can_at91sam7a3_conf * can_conf;

	/* Validate config size */
	if (conf != NULL && conflen == sizeof(struct can_at91sam7a3_conf)) {
		can_conf = (struct can_at91sam7a3_conf *)conf;
		bitrate = can_conf->bitrate;
		clock_speed = can_conf->clock_speed;
	} else {
		return -1;
	}

	/* Set callbacks */
	txcb = atxcb;
	rxcb = arxcb;

	/* Enable CAN Clock */
	AT91C_BASE_PMC->PMC_PCER = (0x1 << AT91C_ID_CAN0);

	/* TODO: Ensure CAN0 pins are not used for PIO */

	/* Configure baudrate */
	CAN_CTRL->BR = CAN_MODE(bitrate, clock_speed);
	printf("Baudrate register: %"PRIx32"\r\n", CAN_CTRL->BR);

	/* The AT91SAM7A1 uses binary '1' to mark don't care bits */
	mask = ~mask;
	can_init_interrupt(id, mask);

	/* Enable CAN in Control Register  */
	CAN_CTRL->MR = CANEN;

	return 0;

}

int can_send(can_id_t id, uint8_t data[], uint8_t dlc, CSP_BASE_TYPE * task_woken) {

	int i, m = -1;
	uint32_t temp[2];

	/* Disable interrupts while looping mailboxes */
	if (task_woken == NULL) {
		portENTER_CRITICAL();
	}

	/* Find free mailbox */
	for(i = 0; i < CAN_TX_MBOX; i++) {
		//if (mbox[i] == MBOX_FREE && !(CAN_CTRL->CHANNEL[i].CR & CHANEN)) {
		if (mbox[i] == MBOX_FREE) {
			mbox[i] = MBOX_USED;
			m = i;
			break;
		}
	}

	/* Enable interrupts */
	if (task_woken == NULL) {
		portEXIT_CRITICAL();
	}

	/* Return if no available MOB was found */
	if (m < 0) {
		csp_debug(CSP_ERROR, "TX overflow, no available MOB\r\n");
		return -1;
	}

	printf("Got mob %d\r\n", m);

	/* Copy 29 identifier to IR register */
	CAN_CTRL->CHANNEL[m].MID = (id & 0x1FFFFFFF) | MIDE;

	/* Copy data to MDH and MDL registers */
	switch (dlc) {
		case 8:
			*(((uint8_t *) &(temp[1])) + 3) = data[7];
		case 7:
			*(((uint8_t *) &(temp[1])) + 2) = data[6];
		case 6:
			*(((uint8_t *) &(temp[1])) + 1) = data[5];
		case 5:
			*(((uint8_t *) &(temp[1])) + 0) = data[4];
		case 4:
			*(((uint8_t *) &(temp[0])) + 3) = data[3];
		case 3:
			*(((uint8_t *) &(temp[0])) + 2) = data[2];
		case 2:
			*(((uint8_t *) &(temp[0])) + 1) = data[1];
		case 1:
			*(((uint8_t *) &(temp[0])) + 0) = data[0];
		default:
			break;
	}

	CAN_CTRL->CHANNEL[m].MDH = temp[0];
	CAN_CTRL->CHANNEL[m].MDL = temp[1];

	/* Set IDE bit, PCB to producer, DLC and CHANEN to enable */
	CAN_CTRL->CHANNEL[m].MCR = (((dlc & 0x0F) << 16) | MTCR);

    return 0;

}

/* Deferred Service Routine */
static void __attribute__ ((noinline)) can_dsr(void) {

	uint8_t m;
	portBASE_TYPE task_woken = pdFALSE;

	/* Run through the mailboxes */
	for (m = 0; m < CAN_MBOXES; m++) {

		/* Check for event */
		if (CAN_CTRL->SR & (1 << m)) {
			printf("Event on mailbox %d\r\n", m);

			/* Message ready */
			if (CAN_CTRL->CHANNEL[m].MCR & MRDY) {
				printf("MRDY on mailbox %d\r\n", m);

				if (m >= CAN_TX_MBOX) {
					printf("Mailbox %d RX OK\r\n", m);
					/* RX mailbox */
					can_frame_t frame;

					if (m == CAN_MBOXES - 1) {
						/* RX overflow */
						csp_debug(CSP_ERROR, "RX Overflow!\r\n");
					} else {
						/* Read DLC */
						frame.dlc = (uint8_t)((CAN_CTRL->CHANNEL[m].MCR >> 16) & 0x0F);

						/* Read data */
						frame.data32[0] = (CAN_CTRL->CHANNEL[m].MDH);
						frame.data32[1] = (CAN_CTRL->CHANNEL[m].MDL);

						/* Read identifier */
						frame.id = (CAN_CTRL->CHANNEL[m].MID & 0x1FFFFFFF);

						/* Call RX Callback */
						if (rxcb != NULL)
							rxcb(&frame, &task_woken);
					}

					/* Get ready to receive new mail */
					CAN_CTRL->CHANNEL[m].MCR = MTCR;
				} else {
					printf("Mailbox %d TX OK\r\n", m);
					/* TX mailbox */

					/* Get identifier */
					can_id_t id = (CAN_CTRL->CHANNEL[m].MID & 0x1FFFFFFF);

					/* Call TX Callback with no error */
					if (txcb != NULL)
						txcb(id, CAN_NO_ERROR, &task_woken);

					/* Release mailbox */
					mbox[m] = MBOX_FREE;
				}
			}
		}
	}

	/* Yield if required */
	if (task_woken == pdTRUE)
		portYIELD_FROM_ISR();

}

/** Low-level IRQ handler
 * Masks the interrupt and acknowledges it. Then lets
 * the deferred service routine handle the data.
 */
static void __attribute__((naked)) can_isr(void) {
	/* Save context */
	portSAVE_CONTEXT();

	/* Call DSR */
	can_dsr();

	/* Acknowledge interrupt */
	AT91C_BASE_AIC->AIC_EOICR = 1;

	/* Restore context */
	portRESTORE_CONTEXT();
}
