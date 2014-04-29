/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
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

/* AT91SAM7A1 driver */

#include <dev/arm/at91sam7.h>

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <FreeRTOS.h>
#include <task.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

#include "can.h"
#include "can_at91sam7a1.h"

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
 *  Fixed values: PROP=0 SJW=1 SMP=1 PHSEG1=3 PHSEG2=3 */
#define CAN_MODE(bitrate,clock_speed) (0x335000 | ((clock_speed)/(10 * (bitrate)) - 1))

/** Pointers to the hardware */
can_controller_t * const CAN_CTRL = ((can_controller_t *) CAN_BASE_ADDRESS);

/** ISR prototype */
static void can_isr(void);

/** Setup CAN interrupts */
static void can_init_interrupt(uint32_t id, uint32_t mask) {
	uint8_t mbox;

	/* Configure ISR */
	AT91F_AIC_ConfigureIt(AT91C_BASE_AIC, AT91C_ID_CAN,
			AT91C_AIC_PRIOR_HIGHEST, AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE,
			(void(*)(void)) can_isr);

	/* Switch off interrupts */
	CAN_CTRL->IDR = (BUSOFF | ERPAS | ENDINIT);

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
			CAN_CTRL->CHANNEL[mbox].IER = (ACK | FRAME | CRC | STUFF | BUS | RXOK);
		} else {
			/* Setup the wanted interrupt mask in the EIR register */
			CAN_CTRL->CHANNEL[mbox].IER = (ACK | FRAME | CRC | STUFF | BUS | TXOK);
		}

		/* Enable the interrupt in the SIER register */
		CAN_CTRL->SIER = (1 << mbox);
	}

	/* Enable interrupt */
	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_CAN);

}

int can_init(uint32_t id, uint32_t mask, can_tx_callback_t atxcb, can_rx_callback_t arxcb, struct csp_can_config *conf) {

	int i;

	csp_assert(conf && conf->bitrate && conf->clock_speed);

	/* Set callbacks */
	txcb = atxcb;
	rxcb = arxcb;

	/* Enable CAN Clock */
	CAN_CTRL->ECR = CAN;

	/* CAN Software Reset */
	CAN_CTRL->CR = SWRST;

	/* Reenable CAN Clock after reset */
	CAN_CTRL->ECR = CAN;

	/* Wait for CAN hardware state machine initialisation */
	/* This avoid using the interrupt that should take more time to process */
	for (i = 0; i < (8 * 32); i++)
		;

	/* Configure CAN Mode (Baudrate) */
	CAN_CTRL->MR = CAN_MODE(conf->bitrate, conf->clock_speed);

	/* The AT91SAM7A1 uses binary '1' to mark don't care bits */
	mask = ~mask;
	can_init_interrupt(id, mask);

	/* Enable CAN in Control Register  */
	CAN_CTRL->CR = CANEN;

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
		if (mbox[i] == MBOX_FREE && !(CAN_CTRL->CHANNEL[i].CR & CHANEN)) {
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
		csp_log_error("TX overflow, no available MOB\r\n");
		return -1;
	}

	/* Copy 29 identifier to IR register */
	CAN_CTRL->CHANNEL[m].IR = (((id & 0x1FFC0000) >> 18)
			| ((id & 0x3FFFF) << 11));

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

	/* Clear status register to remove any old status flags */
	CAN_CTRL->CHANNEL[m].CSR = 0xFFF;

	/* Set IDE bit, PCB to producer, DLC and CHANEN to enable */
	CAN_CTRL->CHANNEL[m].CR = (CHANEN | PCB | IDE | dlc);

	return 0;

}

static void __attribute__ ((noinline)) can_dsr(void) {

	uint8_t m;
	portBASE_TYPE task_woken = pdFALSE;

	/* Run through the mailboxes */
	for (m = 0; m < CAN_MBOXES; m++) {

		/* If an interrupt occurred on mailbox */
		if (CAN_CTRL->ISSR & (1 << m)) {
			if (CAN_CTRL->CHANNEL[m].SR & RXOK) {
				/* RX Complete */
				can_frame_t frame;

				/* Read DLC */
				frame.dlc = (uint8_t)CAN_CTRL->CHANNEL[m].CR & 0x0F;

				/* Read data */
				frame.data32[0] = CAN_CTRL->CHANNEL[m].DRA;
				frame.data32[1] = CAN_CTRL->CHANNEL[m].DRB;

				/* Read identifier */
				frame.id = ((CAN_CTRL->CHANNEL[m].IR & 0x7FF) << 18)
						| ((CAN_CTRL->CHANNEL[m].IR & 0x1FFFF800) >> 11);

				/* Call RX callback */
				if (rxcb != NULL)
					rxcb(&frame, &task_woken);

				/* Clear status register before enabling */
				CAN_CTRL->CHANNEL[m].CSR = 0xFFF;

				/* Get ready to receive new mail */
				CAN_CTRL->CHANNEL[m].CR = (CHANEN | IDE | DLC);

				/* Finally clear interrupt for mailbox */
				CAN_CTRL->CISR = (1 << m);

			} else if (CAN_CTRL->CHANNEL[m].SR & TXOK) {
				/* TX Complete */

				/* clear status register before enabling */
				CAN_CTRL->CHANNEL[m].CSR = 0xFFF;

				/* Get identifier */
				can_id_t id = ((CAN_CTRL->CHANNEL[m].IR & 0x7FF) << 18)
						| ((CAN_CTRL->CHANNEL[m].IR & 0x1FFFF800) >> 11);

				/* Call TX callback with no error */
				if (txcb != NULL)
					txcb(id, CAN_NO_ERROR, &task_woken);

				/* Disable mailbox */
				CAN_CTRL->CHANNEL[m].CR &= ~(CHANEN);

				/* Release mailbox */
				mbox[m] = MBOX_FREE;

				/* Finally clear interrupt for mailbox */
				CAN_CTRL->CISR = (1 << m);

			} else if (CAN_CTRL->CHANNEL[m].SR != 0) {
				/* Error */
				csp_log_error("mbox %d failed with SR=%#"PRIx32"\r\n",
						m, CAN_CTRL->CHANNEL[m].SR);

				/* Get identifier */
				can_id_t id = ((CAN_CTRL->CHANNEL[m].IR & 0x7FF) << 18)
						| ((CAN_CTRL->CHANNEL[m].IR & 0x1FFFF800) >> 11);

				/* Clear status register before enabling */
				CAN_CTRL->CHANNEL[m].CSR = 0xFFF;

				/* Finally clear interrupt for mailbox */
				CAN_CTRL->CISR = (1 << m);

				/* Call TX callback with error flag set */
				if (txcb != NULL)
					txcb(id, CAN_ERROR, &task_woken);

				/* Disable mailbox */
				CAN_CTRL->CHANNEL[m].CR &= ~(CHANEN);

				/* Release mailbox */
				mbox[m] = MBOX_FREE;

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
