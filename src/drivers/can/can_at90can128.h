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

/* This is a heavily pruned version of the ATMEL CAN header file */

#ifndef _CAN_AT90CAN128_H_
#define _CAN_AT90CAN128_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** General errors interrupt mask */
#define ERR_GEN_MSK 			((1 << SERG) | (1 << CERG) | (1 << FERG) | (1 << AERG) | (1 << BOFFIT))

/** General interrupt mask */
#define INT_GEN_MSK 			((1 << BOFFIT) | (1 << BXOK) | (ERR_GEN_MSK))

/** MOB number mask for HPMOB */
#define HPMOB_MSK   			((1 << HPMOB3) | (1 <<HPMOB2) | (1<<HPMOB1) | (1<<HPMOB0))

/** MOB number mask for CANPAGE */
#define MOBNB_MSK   			((1 << MOBNB3) | (1 <<MOBNB2) | (1<<MOBNB1) | (1<<MOBNB0))

/** MOB errors mask */
#define ERR_MOB_MSK 			((1 << BERR) | (1 << SERR) | (1 << CERR) | (1 << FERR) | (1 << AERR))

/** MOB interrupt mask */
#define INT_MOB_MSK 			((1 << TXOK) | (1 << RXOK) | (1 << BERR) | (1 << SERR) | (1 << CERR) | (1 << FERR) | (1 << AERR))

/** Configuration MOB mask */
#define CONMOB_MSK  			((1 << CONMOB1) | (1 << CONMOB0))

/** Data Lenght Code mask */
#define DLC_MSK	 			((1 << DLC3) | (1 << DLC2) | (1 << DLC1) | (1 << DLC0))

/** MOB status */
#define MOB_NOT_COMPLETED	  	0x00
#define MOB_TX_COMPLETED		(1 << TXOK)
#define MOB_RX_COMPLETED		(1 << RXOK)
#define MOB_RX_COMPLETED_DLCW  	((1 << RXOK) | (1 << DLCW))
#define MOB_ACK_ERROR		   (1 << AERR)
#define MOB_FORM_ERROR		  (1 << FERR)
#define MOB_CRC_ERROR		   (1 << CERR)
#define MOB_STUFF_ERROR		 (1 << SERR)
#define MOB_BIT_ERROR		   (1 << BERR)
#define MOB_PENDING				((1 << RXOK) | (1 << TXOK))
#define MOB_NOT_REACHED			((1 << AERR) | (1 << FERR) | (1 << CERR) | (1 << SERR) | (1 << BERR))
#define MOB_DISABLE		   	0xFF

/** Interrupt control */
#define CAN_SET_INTERRUPT() 	(CANGIE |= (1 << ENIT) | (1 << ENRX) | (1 << ENTX) | (1 << ENERR) | (1 << ENBOFF) | (1 << ENERG))
#define CAN_CLEAR_INTERRUPT() 	(CANGIE &= ~(1 << ENIT))

/** CAN module control */
#define CAN_RESET()	   		(CANGCON = (1 << SWRES))
#define CAN_ENABLE()	  		(CANGCON |= (1 << ENASTB))
#define CAN_DISABLE()	 		(CANGCON &= ~(1 << ENASTB))

#define CAN_FULL_ABORT()  		do {CANGCON |= (1<<ABRQ); \
									CANGCON &= ~(1<<ABRQ);} while (0)

#define CAN_SET_MOB(mob)	   	do { CANPAGE = ((mob) << 4);} while(0)

#define CAN_GET_MOB()			((CANPAGE & 0xF0) >> 4)

#define CAN_CLEAR_STATUS_MOB() 	do {CANSTMOB&=0x00;} while (0)

#define CAN_CLEAR_GENINT()	do {CANGIT|=CANGIT;} while (0);

#define CAN_CLEAR_INT_MOB() 	do {CANSTMOB = (CANSTMOB & ~(1 << RXOK));} while (0)
#define CAN_CLEAR_MOB()			do {uint8_t volatile *__i_; \
									for (__i_=&CANSTMOB; __i_<&CANSTML; __i_++) { \
										*__i_=0x00 ; \
								}} while (0)

/** Highest priority MOB */
#define CAN_HPMOB()		  		((CANHPMOB & 0xF0) >> 4)

#define CAN_MOB_ABORT()   		(DISABLE_MOB)

#define CAN_SET_DLC(dlc)  		(CANCDMOB |= (dlc))
#define CAN_SET_IDE()	 		(CANCDMOB |= (1 << IDE))

#define CAN_CLEAR_DLC()   		(CANCDMOB &= ~DLC_MSK)
#define CAN_CLEAR_IDE()   		(CANCDMOB &= ~(1<<IDE))

#define DISABLE_MOB				(CANCDMOB &= (~CONMOB_MSK))

/** Configure TX MOB */
#define CAN_CONFIG_TX()			do { DISABLE_MOB; \
									CANCDMOB |= (0x01 << CONMOB0); \
								} while (0)

/** Configure RX MOB */
#define CAN_CONFIG_RX()			do { DISABLE_MOB; \
									CANCDMOB |= (0x02 << CONMOB0); \
								} while(0)

/** Identifier macros */
#define CAN_GET_DLC()	 		((CANCDMOB & DLC_MSK) >> DLC0)
#define CAN_GET_IDE()	  		((CANCDMOB & (1 << IDE)) >> IDE)

#define CAN_GET_EXT_ID(_id)  	do { *((uint8_t *)(&(_id))+3) = (uint8_t)(CANIDT1>>3); \
									 *((uint8_t *)(&(_id))+2) = (uint8_t)((CANIDT2>>3)+(CANIDT1<<5)); \
									 *((uint8_t *)(&(_id))+1) = (uint8_t)((CANIDT3>>3)+(CANIDT2<<5)); \
									 *((uint8_t *)(&(_id))  ) = (uint8_t)((CANIDT4>>3)+(CANIDT3<<5)); } while (0)

#define CAN_SET_EXT_ID_28_21(_id)  	(((*((uint8_t *)(&(_id))+3))<<3)+((*((uint8_t *)(&(_id))+2))>>5))
#define CAN_SET_EXT_ID_20_13(_id)  	(((*((uint8_t *)(&(_id))+2))<<3)+((*((uint8_t *)(&(_id))+1))>>5))
#define CAN_SET_EXT_ID_12_5(_id)  	(((*((uint8_t *)(&(_id))+1))<<3)+((* (uint8_t *)(&(_id))   )>>5))
#define CAN_SET_EXT_ID_4_0(_id)  	 ((* (uint8_t *)(&(_id))   )<<3)

#define CAN_SET_EXT_ID(_id)  	do { CANIDT1   = CAN_SET_EXT_ID_28_21(_id); \
									 CANIDT2   = CAN_SET_EXT_ID_20_13(_id); \
									 CANIDT3   = CAN_SET_EXT_ID_12_5( _id); \
									 CANIDT4   = CAN_SET_EXT_ID_4_0(  _id); \
									 CANCDMOB |= (1<<IDE);						 } while (0)

#define CAN_SET_EXT_MSK(mask)   do { CANIDM1   = CAN_SET_EXT_ID_28_21(mask); \
									 CANIDM2   = CAN_SET_EXT_ID_20_13(mask); \
									 CANIDM3   = CAN_SET_EXT_ID_12_5( mask); \
									 CANIDM4   = CAN_SET_EXT_ID_4_0(  mask); } while (0)

#define CAN_CLEAR_IDEMSK() 	(CANIDM4 &= ~(1<<IDEMSK))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CAN_AT90CAN128_H_ */
