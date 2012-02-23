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

#ifndef _CAN_AT91SAM7A3_H_
#define _CAN_AT91SAM7A3_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** CAN controller base address */
#define CAN0_BASE_ADDRESS 0xFFF80000
#define CAN1_BASE_ADDRESS 0xFFF84000

/** CAN Channel Structure */
typedef struct {
   uint32_t	 MMR;	 		 	/* Mailbox Mode Register				*/
   uint32_t	 MAM;				/* Mailbox Acceptance Mask Register	 */
   uint32_t	 MID;				/* Mailbox ID Register			 		*/
   uint32_t	 MFID;			   /* Mailbox Family ID Register			*/
   uint32_t	 MSR;		  		/* Mailbox Status Register				*/
   uint32_t	 MDL;				/* Mailbox Data Low Register		 	*/
   uint32_t	 MDH;				/* Mailbox Date High Register		  	*/
   uint32_t	 MCR;		  		/* Mailbox Control Register				*/
} volatile can_channel_t;

/** CAN Controller Structure 16 Channels */
typedef struct {
   uint32_t  MR;					/* Mode Register					   	*/
   uint32_t  IER;					/* Interrupt Enable Register		  	*/
   uint32_t  IDR;					/* Interrupt Disable Register		 	*/
   uint32_t  IMR;					/* Interrupt Mask Register				*/
   uint32_t  SR;			 		/* Status Register				  	*/
   uint32_t  BR;			 		/* Baudrate Register					*/
   uint32_t  TIM;					/* Timer Register					  	*/
   uint32_t  TIMESTP;		  		/* Timestamp Register			   	*/
   uint32_t  ECR;			 		/* Error Counter Register		 		*/
   uint32_t  TCR;					/* Tranfer Counter Register				*/
   uint32_t  ACR;					/* Abort Command Register		   	*/
   uint32_t  Reserved[117];		 /* Reserved		 					*/
   can_channel_t CHANNEL[16]; 		/* CAN Channels					   	*/
} volatile can_controller_t;

/** CAN Mode Register: MR */
#define  CANEN		(0x01 << 0)   	/* CAN Controller Enable				*/
#define  LPM	 	(0x01 << 1)   	/* Disable/Enable Low Power Mode  		*/
#define  ABM	  	(0x01 << 2)  	/* Disable/Enable Autobaud/Listen mode 	*/
#define  OVL	  	(0x01 << 3)  	/* Disable/Enable Overload Frame 		*/
#define  TEOF   	(0x01 << 4)  	/* Timestamp messages at each end of Frame */
#define  TTM   		(0x01 << 5)  	/* Disable/Enable Time Triggered Mode 	*/
#define  TIMFRZ   	(0x01 << 6)  	/* Enable Timer Freeze 					*/
#define  DRPT   	(0x01 << 7)  	/* Disable Repeat 						*/

/** CAN Interrupt Registers: CSR, SR, IER, IDR, IMR, ACR */
#define  MB0   		(0x01 << 0)   	/* Mailbox 0 Interrupt Enable 			*/
#define  MB1   		(0x01 << 1)   	/* Mailbox 1 Interrupt Enable 			*/
#define  MB2   		(0x01 << 2)   	/* Mailbox 2 Interrupt Enable 			*/
#define  MB3   		(0x01 << 3)   	/* Mailbox 3 Interrupt Enable 			*/
#define  MB4   		(0x01 << 4)   	/* Mailbox 4 Interrupt Enable 			*/
#define  MB5   		(0x01 << 5)   	/* Mailbox 5 Interrupt Enable 			*/
#define  MB6   		(0x01 << 6)   	/* Mailbox 6 Interrupt Enable 			*/
#define  MB7   		(0x01 << 7)   	/* Mailbox 7 Interrupt Enable 			*/
#define  MB8   		(0x01 << 8)   	/* Mailbox 8 Interrupt Enable 			*/
#define  MB9   		(0x01 << 9)   	/* Mailbox 9 Interrupt Enable 			*/
#define  MB10   	(0x01 << 10)   	/* Mailbox 10 Interrupt Enable 			*/
#define  MB11   	(0x01 << 11)   	/* Mailbox 11 Interrupt Enable 			*/
#define  MB12   	(0x01 << 12)   	/* Mailbox 12 Interrupt Enable 			*/
#define  MB13   	(0x01 << 13)   	/* Mailbox 13 Interrupt Enable 			*/
#define  MB14   	(0x01 << 14)   	/* Mailbox 14 Interrupt Enable 			*/
#define  MB15   	(0x01 << 15)   	/* Mailbox 15 Interrupt Enable 			*/
#define  ERRA  		(0x01 << 16)   	/* Error Active mode Interrupt Enable  	*/
#define  WARN  		(0x01 << 17)   	/* Warning Limit Interrupt Enable 		*/
#define  ERRP		(0x01 << 18)   	/* Error Passive mode Interrupt Enable	*/
#define  BOFF   	(0x01 << 19)   	/* Bus-off mode Interrupt Enable 		*/
#define  SLEEP	 	(0x01 << 20)   	/* Sleep Interrupt Enable 				*/
#define  WAKEUP		 (0x01 << 21)   	/* Wakeup Interrupt Enable 				*/
#define  TOVF	  	(0x01 << 22)   	/* Timer Overflow Interrupt Enable 		*/
#define  TSTP	  	(0x01 << 23)  	/* TimeStamp Interrupt Enable			*/
#define  CERR	  	(0x01 << 24)  	/* CRC Error Interrupt Enable 			*/
#define  SERR	  	(0x01 << 25)  	/* Stuffing Error Interrupt Enable 		*/
#define  AERR	  	(0x01 << 26)  	/* Acknowledgment Error Interrupt Enable */
#define  FERR	  	(0x01 << 27)  	/* Form Error Interrupt Enable 			*/
#define  BERR	  	(0x01 << 28)  	/* Bit Error Interrupt Enable 			*/
#define  RBSY		(0x01 << 29) 	/* Receiver busy */
#define  TBSY		(0x01 << 30) 	/* Transmitter busy */
#define  OVLSY		(0x01 << 31) 	/* Overload busy */

/** CAN Baudrate Register: BR */
#define  PHASE2   	(0x07 << 0)  	/* Phase 2 segment  				  	*/
#define  PHASE1   	(0x07 << 4)  	/* Phase 1 segment						*/
#define  PROPAG		(0x07 << 8)   	/* Propagation Time Segment				*/
#define  SWJ	  	(0x03 << 12)  	/* Re-synchronization jump width 		*/
#define  BRP	 	(0x7F << 16)   	/* Baudrate Prescaler 					*/
#define  SMP	  	(0x01 << 24)  	/* Sampling Mode						*/

/** CAN Timer Register: TIM */
#define  TIMER   	(0xffff << 0)  	/* Timer				  				*/

/** CAN Timestamp Register: TIMESTP */
#define  MTIMESTAMP (0xffff << 0)  	/* Timestamp			  				*/

/** CAN Timestamp Register: TIMESTP */
#define  REC 		(0xffff << 0)  	/* Receive Error Counter 				*/
#define  TEC 		(0xffff << 16)  /* Transmit Error Counter 				*/

/** CAN Transfer Command Register: TCR */
#define  TIMRST		(0x01 << 31)   	/* Timer Reset 							*/

/** CAN Message Mode Register: MMR */
#define  TMIMEMARK 	(0xFFFF << 0)   /* Mailbox Timemark 				*/
#define  PRIOR		(0x0F << 16)	/* Mailbox Priority 				*/
#define  MOT		(0x07 << 24)	/* Mailbox Object Type 				*/

/** CAN Message Register: MAM, MID */
#define  MIDvB 		(0x3FFFF << 0)  /* Complementary bits for identifier in extended frame mode */
#define  MIDvA 		(0x7FF << 18)   /* Identifier for standard frame mode */
#define  MIDE 		(0x01 << 29)   	/* Mailbox Timemark 				*/

/** CAN Message Family ID Register */
#define  MFID 		(0x7FFFFFF << 0)  /* Family ID */

/** CAN Message Status Register: MSR */
/* MTIMESTAMP defined in TIMSTP */  /* Timer value 						*/
#define  MDLC		(0x0F << 16)	/* Mailbox Data Length Code 		*/
#define  MRTR		(0x01 << 20)	/* Mailbox Remote Transmission Request */
#define  MABT		(0x01 << 22)	/* Mailbox Message Abort */
#define  MRDY		(0x01 << 23)	/* Mailbox Ready */
#define  MMI		(0x07 << 24)	/* Mailbox Message Ignored */

/** CAN Message Data Low Register: MDL */
//#define  MDL		(0xFFFFFFFF << 0)   /* Message Data Low Value */

/** CAN Message Data High Register: MDH */
//#define  MDH		(0xFFFFFFFF << 0)   /* Message Data High Value */

/** CAN Message Control Register: MCR */
#define  MACR		(0x01 << 22)   	/* Mailbox Abort Request */
#define  MTCR		(0x01 << 23) 	/* Mailbox Transfer Command */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CAN_AT91SAM7A3_H_ */
