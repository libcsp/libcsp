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

/* AT91SAM7A1 driver */

/* IMPORTANT : ALLWAYS USE RX MAILBOXES WITH HIGHER NUMBER THAN TX MAILBOX */
/*             OTHERWISE THE TX MAILBOX WILL TRANSMIT ALL DATA IN RX       */
/*             MAILBOXES WITH A LOWER NUMBER (A bug in the ATAC-D chip)    */

#include <dev/arm/at91sam7.h>

#include <stdio.h>
#include <inttypes.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

#include "can.h"
#include "can_at91sam7a1.h"

/* MOB segmentation */
#define CAN_RX_MBOX 8
#define CAN_TX_MBOX 8
#define CAN_MBOXES 	16

/** Callback functions */
can_tx_callback_t txcb;
can_rx_callback_t rxcb;

/** Identifier and mask */
uint32_t can_id;
uint32_t can_mask;

/** Calculate BD field of mode register. Each bit is divided in 10 time quanta */
#define CAN_MR_BD(bitrate,clock_speed) ((clock_speed)/(10 * (bitrate)) - 1)

/* ISR prototype */
static void can_isr(void);

/** Setup CAN interrupts */
static void can_init_interrupt(uint32_t id, uint32_t mask) {
	uint8_t mbox;

	/* Configure ISR */
	AT91F_AIC_ConfigureIt(AT91C_BASE_AIC, AT91C_ID_CAN,
			AT91C_AIC_PRIOR_HIGHEST, AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE,
			(void(*)(void)) can_isr);

	/* Switch off interrupts */
	CAN_CTRL->IER = 0;

	/* Enable interrupts for all mailboxes */
	for (mbox = 0; mbox < CAN_MBOXES; mbox++) {
		if (mbox >= CAN_TX_MBOX) {
			/* Setup the mask and ID */
			CAN_CTRL->CHANNEL[mbox].MSK = ((mask & 0x1FFC0000) >> 18)
					| ((mask & 0x3FFFF) << 11);
			CAN_CTRL->CHANNEL[mbox].IR = ((id & 0x1FFC0000) >> 18)
					| ((id & 0x3FFFF) << 11);

			/* get the mailbox ready to receive CAN telegrams */
			CAN_CTRL->CHANNEL[mbox].CR = (CHANEN | IDE | DLC);

			/* setup the wanted interrupt mask in the EIR register */
			CAN_CTRL->CHANNEL[mbox].IER = RXOK;
		} else {
			/* Setup the wanted interrupt mask in the EIR register */
			CAN_CTRL->CHANNEL[mbox].IER = TXOK;
		}

		/* Enable the interrupt in the SIER register */
		CAN_CTRL->SIER = (1 << mbox);
	}

	/* Enable interrupt */
	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_CAN);
}

int can_init(uint32_t id, uint32_t mask, can_tx_callback_t atxcb, can_rx_callback_t arxcb, void * conf, int conflen) {

	int i;
	uint32_t bitrate;
	uint32_t clock_speed;
	struct can_at90can128_conf * can_conf;

	/* Validate config size */
	if (conf != NULL && conflen == sizeof(struct can_at90can128_conf)) {
		can_conf = (struct can_at90can128_conf *)conf;
		bitrate = can_conf->bitrate;
		clock_speed = can_conf->clock_speed;
	} else {
		return -1;
	}

	/* Enable CAN Clock */
	CAN_CTRL->ECR = CAN;

	/* Enable CAN in Control Register  */
	CAN_CTRL->CR = CANEN;

	/* CAN Software Reset */
	CAN_CTRL->CR = SWRST;

	/* wait for CAN hardware state machine initialisation */
	/* This avoid using the interrupt that should take more time to process */
	for (i = 0; i < (8 * 32); i++)
		;

	/* Configure CAN Mode (Baudrate) */
	CAN_CTRL->MR = 0x335000 | CAN_MR_BD(bitrate, clock_speed);
	printf("CAN MR: %#010"PRIx32"\r\n", CAN_CTRL->MR);

	can_init_interrupt(id, mask);

	return 0;

}

int can_send(can_id_t id, uint8_t data[], uint8_t dlc) {

	int i, m = -1;

	/* Search free MOB from 0 -> CAN_TX_MOBs */
	for(i = 0; i < CAN_TX_MBOX; i++) {
		if ((CAN_CTRL->CHANNEL[i].CR & CHANEN) == 0) {
			/* TODO: Mark mbox used in a locked array */
			i = m;
			break;
		}
	}

	/* Return if no available MOB was found */
	if (m < 0) {
		csp_debug(CSP_ERROR, "TX overflow, no available MOB\r\n");
		return -1;
	}

	uint32_t temp[2];

	/* Copy 29 identifier to IR register */
	CAN_CTRL->CHANNEL[m].IR = (((id & 0x1FFC0000) >> 18) | ((id & 0x3FFFF) << 11));

	/* Copy data to CAN mailbox DRAx and DRBx registers */
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

	CAN_CTRL->CHANNEL[m].DRA = temp[0];
	CAN_CTRL->CHANNEL[m].DRB = temp[1];

	/* Set IDE bit, PCB to producer, DLC and CHANEN to enable */
	CAN_CTRL->CHANNEL[m].CR = (CHANEN | PCB | IDE | dlc);

	/* Clear status register to remove any old status flags */
	CAN_CTRL->CHANNEL[m].CSR = 0xFFF;

    return 0;

}

__attribute__ ((noinline)) static void can_dsr() {

	uint8_t mbox;
	uint32_t temp[2];
	portBASE_TYPE task_woken;

	/* Run through the mailboxes */
	for (mbox = 0; mbox < 16; mbox++) {

		if (CAN_CTRL->CHANNEL[mbox].SR & RXOK) {
			/* RX Complete */
			can_frame_t frame;

			if (mbox == CAN_MBOXES - 1) {
				/* RX overflow */
				csp_debug(CSP_ERROR, "RX Overflow!\r\n");
				/* TODO: Handle this */
			}

			/* Read DLC */
			frame.dlc = (uint8_t)CAN_CTRL->CHANNEL[mbox].CR & 0x0F;

			/* Copy data - all 8 databytes will be fetched */
			/* It must be done like this due to struct data alignment */
			temp[0] = CAN_CTRL->CHANNEL[mbox].DRA;
			temp[1] = CAN_CTRL->CHANNEL[mbox].DRB;
			switch (frame.dlc) {
				case 8:
					frame.data[7] = *((uint8_t *)(&(temp[1]) + 3));
				case 7:
					frame.data[6] = *((uint8_t *)(&(temp[1]) + 2));
				case 6:
					frame.data[5] = *((uint8_t *)(&(temp[1]) + 1));
				case 5:
					frame.data[4] = *((uint8_t *)(&(temp[1]) + 0));
				case 4:
					frame.data[3] = *((uint8_t *)(&(temp[0]) + 3));
				case 3:
					frame.data[2] = *((uint8_t *)(&(temp[0]) + 2));
				case 2:
					frame.data[1] = *((uint8_t *)(&(temp[0]) + 1));
				case 1:
					frame.data[0] = *((uint8_t *)(&(temp[0]) + 0));
				default:
					break;
			}

			/* Get identifier */
			frame.id = ((CAN_CTRL->CHANNEL[mbox]. IR & 0x7FF) << 18)
					| ((CAN_CTRL->CHANNEL[mbox]. IR & 0x1FFFF800) >> 11);

			/* Clear status register before enabling */
			CAN_CTRL->CHANNEL[mbox].CSR = 0xFFF;

			/* Get ready to receive new mail */
			CAN_CTRL->CHANNEL[mbox].CR = (CHANEN | IDE | DLC);

		} else if (CAN_CTRL->CHANNEL[mbox].SR & TXOK) {
			/* TX Complete */
			can_id_t id;

			/* clear status register before enabling */
			CAN_CTRL->CHANNEL[mbox].CSR = 0xFFF;

			/* Get identifier */
			id = ((CAN_CTRL->CHANNEL[mbox]. IR & 0x7FF) << 18)
					| ((CAN_CTRL->CHANNEL[mbox]. IR & 0x1FFFF800) >> 11);

			/* Do TX-Callback */
			if (txcb != NULL) txcb(id, &task_woken);

		} else if (CAN_CTRL->CHANNEL[mbox].SR != 0) {
			/* Error */
			/* Do something about other interrupts */
			/* clear status register before enabling */
			CAN_CTRL->CHANNEL[mbox].CSR = 0xFFF;
		}

		/* Finally clear interrupt for mailbox */
		CAN_CTRL->CISR = 1 << mbox;
	}

	if (task_woken == pdTRUE)
		portYIELD_FROM_ISR();

}

/** Low-level IRQ handler
 * Masks the interrupt and acknowledges it. Then lets
 * the deferred service routine handle the data. In and out.
 */
__attribute__((naked)) static void can_isr() {
	/* Save context */
	portSAVE_CONTEXT();

	/* Call DSR */
	can_dsr();

	/* Acknowledge interrupt */
	AT91C_BASE_AIC->AIC_EOICR = 1;

	/* Restore context */
	portRESTORE_CONTEXT();
}
