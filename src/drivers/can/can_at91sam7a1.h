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

#ifndef _CAN_AT91SAM7A1_H_
#define _CAN_AT91SAM7A1_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** CAN controller base address */
#define CAN_BASE_ADDRESS 0xFFFD4000

/** CAN Channel Structure */
typedef struct {
   uint32_t  ReservedA[5];   		/* Reserved						   	*/
   uint32_t  DRA;					/* Data Register A					  */
   uint32_t  DRB;					/* Data Register B					  */
   uint32_t  MSK;					/* Mask Register						*/
   uint32_t  IR;			 		/* Identifier Register				  */
   uint32_t  CR;			 		/* Control Register					 */
   uint32_t  STP;					/* Stamp Register					   */
   uint32_t  CSR;					/* Clear Status Register				*/
   uint32_t  SR;			 		/* Status Register					  */
   uint32_t  IER;					/* Interrupt Enable Register			*/
   uint32_t  IDR;					/* Interrupt Disable Register		   */
   uint32_t  IMR;					/* Interrupt Mask Register		 		*/
} volatile can_channel_t;

/** CAN Controller Structure 16 Channels */
typedef struct {
   uint32_t	 ReservedA[20];	  /* Reserved								*/
   uint32_t	 ECR;				/* Enable Clock Register			   	*/
   uint32_t	 DCR;				/* Disable Clock Register			  	*/
   uint32_t	 PMSR;			   /* Power Management Status Register		*/
   uint32_t	 ReservedB;		  /* Reserved								*/
   uint32_t	 CR;				 /* Control Register						*/
   uint32_t	 MR;				 /* Mode Register						*/
   uint32_t	 ReservedC;		  /* Reserved							 */
   uint32_t	 CSR;				/* Clear Status Register				*/
   uint32_t	 SR;				 /* Status Register					  */
   uint32_t	 IER;				/* Interrupt Enable Register			*/
   uint32_t	 IDR;				/* Interrupt Disable Register		   */
   uint32_t	 IMR;			   	/* Interrupt Mask Register			  */
   uint32_t	 CISR;			  	/* Clear Interrupt Source Register	  */
   uint32_t	 ISSR;			  	/* Interrupt Source Status Register	 */
   uint32_t	 SIER;			  	/* Source Interrupt Enable Register	 */
   uint32_t	 SIDR;			  	/* Source Interrupt Disable Register	*/
   uint32_t	 SIMR;		  		/* Source Interrupt Mask Register	   */
   uint32_t	 ReservedD[22];	 	/* Reserved							 */
   can_channel_t CHANNEL[16]; 		/* CAN Channels					   	*/
} volatile can_controller_t;

/** CAB Clock Registers:  ECR, DCR, PMSR */
#define  CAN	  	(0x01 << 1)   	/* CAN Clock						 	*/

/** CAN Control Register: CR */
#define  SWRST		(0x01 << 0)   	/* CAN Software Reset					*/
#define  CANEN		(0x01 << 1)   	/* CAN Enable						 	*/
#define  CANDIS   	(0x01 << 2)   	/* CAN Disable						 	*/
#define  ABEN	 	(0x01 << 3)   	/* Abort Request Enable					*/
#define  ABDIS		(0x01 << 4)   	/* Abort Request Disable				*/
#define  OVEN	 	(0x01 << 5)   	/* Overload Request Enable			  */
#define  OVDIS		(0x01 << 6)   	/* Overload Request Disable			 */

/** CAN Mode Register: MR */
#define  BD	   	(0x1F << 0)   	/* Baudrate Prescalar				   */
#define  PROP	 	(0x07 << 8)   	/* Propagation Segment Value			*/
#define  SWJ	  	(0x03 << 12)  	/* Synchronization With Jump			*/
#define  SMP	  	(0x01 << 14)  	/* Sampling Mode						*/
#define  PHSEG1   	(0x07 << 16)  	/* Phase Segment 1 Value				*/
#define  PHSEG2   	(0x07 << 20)  	/* Phase Segment 2 Value				*/

/** CAN Interrupt Registers: CSR, SR, IER, IDR, IMR */
#define  CANENA   	(0x01 << 0)   	/* CAN Enable						   */
#define  CANINIT  	(0x01 << 1)   	/* CAN Initialized					  */
#define  ENDINIT  	(0x01 << 2)   	/* End of CAN Initialization			*/
#define  ERPAS		(0x01 << 3)   	/* Error Passive						*/
#define  BUSOFF   	(0x01 << 4)   	/* Bus Off							  */
#define  ABRQ	 	(0x01 << 5)   	/* CAN Abort Request					*/
#define  OVRQ	 	(0x01 << 6)   	/* CAN Overload Request				 */
#define  ISS	  	(0x01 << 7)   	/* Interrupt Source Status			  */
#define  REC	  	(0x0F << 16)  	/* Reception Error Counter			  */
#define  TEC	  	(0x0F << 24)  	/* Transmit Error Counter			   */

/** CAN Channel Data Registers: DRA, DRB */
#define  DATA0	 	(0x0F << 0)   	/* Data 0							   */
#define  DATA1	 	(0x0F << 8)   	/* Data 1							   */
#define  DATA2	 	(0x0F << 16)  	/* Data 2							   */
#define  DATA3	 	(0x0F << 24)  	/* Data 3							   */
#define  DATA4	 	(0x0F << 0)   	/* Data 4							   */
#define  DATA5	 	(0x0F << 8)   	/* Data 5							   */
#define  DATA6	 	(0x0F << 16)  	/* Data 6							   */
#define  DATA7	 	(0x0F << 24)  	/* Data 7							   */

/** CAN Channel Mask Register: MSK */
#define  MASK	  	(0xFFFFFF << 0)   /* Identifier Mask					*/
#define  MRB	   	(0x03	 << 29)  /* Reserved Mask Bits				 */
#define  MRTR	  	(0x01	 << 31)  /* Remote Transmission Resquest Mask  */

/** CAN Channel Identifier Register: IR */
#define  ID			(0x1FFFFFFF << 0)   /* Identifier					   */
#define  RESB	  	(0x03	   << 29)  /* Reserved Bits					*/
#define  RTR	   	(0x01	   << 31)  /* Remote Transmission Resquest	 */

/** CAN Channel Control Register: CR */
#define  DLC	   	(0x01 << 3)   /* Data Length Code , 8				   */
#define  IDE	   	(0x01 << 4)   /* Extended Identifier Flag			   */
#define  RPLYV	 	(0x01 << 5)   /* Automatic Reply						*/
#define  PCB	   	(0x01 << 6)   /* Channel Producer					   */
#define  CHANEN		(0x01 << 7)   /* Channel Enable						 */
#define  OVERWRITE 	(0x01 << 8)   /* Frame Overwrite						*/

/** CAN Channel Interrupt Registers: CSR, SR, IER, IDR, IMR */
#define  ACK	   	(0x01 << 0)   /* Acknowledge Error					  */
#define  CAN_FRAME 	(0x01 << 1)   /* Frame Error							*/
#define  CRC	   	(0x01 << 2)   /* CRC Error							  */
#define  STUFF	 	(0x01 << 3)   /* Stuffing Error						 */
#define  BUS	   	(0x01 << 4)   /* Bus Error							  */
#define  RXOK  		(0x01 << 5)   /* Reception Completed					*/
#define  TXOK	   (0x01 << 6)   /* Transmission Completed				 */
#define  RFRAME	 (0x01 << 7)   /* Remote Frame						   */
#define  WRERROR	(0x01 << 8)   /* Write Error							*/
#define  DLCW	   (0x01 << 9)   /* DLC Warning							*/
#define  FILLED	 (0x01 << 10)  /* Reception buffer filled				*/
#define  OVRUN	  (0x01 << 11)  /* Overrun								*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CAN_AT91SAM7A1_H_ */
