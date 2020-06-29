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

#ifndef _CSP_ERROR_H_
#define _CSP_ERROR_H_

/**
   @file

   Error codes.
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
   @defgroup CSP_ERR CSP error codes.
   @{
*/
#define CSP_ERR_NONE		 0		/**< No error */
#define CSP_ERR_NOMEM		-1		/**< Not enough memory */
#define CSP_ERR_INVAL		-2		/**< Invalid argument */
#define CSP_ERR_TIMEDOUT	-3		/**< Operation timed out */
#define CSP_ERR_USED		-4		/**< Resource already in use */
#define CSP_ERR_NOTSUP		-5		/**< Operation not supported */
#define CSP_ERR_BUSY		-6		/**< Device or resource busy */
#define CSP_ERR_ALREADY		-7		/**< Connection already in progress */
#define CSP_ERR_RESET		-8		/**< Connection reset */
#define CSP_ERR_NOBUFS		-9		/**< No more buffer space available */
#define CSP_ERR_TX		-10		/**< Transmission failed */
#define CSP_ERR_DRIVER		-11		/**< Error in driver layer */
#define CSP_ERR_AGAIN		-12		/**< Resource temporarily unavailable */
#define CSP_ERR_HMAC		-100		/**< HMAC failed */
#define CSP_ERR_XTEA		-101		/**< XTEA failed */
#define CSP_ERR_CRC32		-102		/**< CRC32 failed */
#define CSP_ERR_SFP		-103		/**< SFP protocol error or inconsistency */
/**@}*/

#ifdef __cplusplus
}
#endif
#endif
