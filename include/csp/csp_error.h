/*****************************************************************************
 * **File:** csp/csp_error.h
 *
 * **Description:** Error codes.
 ****************************************************************************/
#pragma once

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
#define CSP_ERR_NOSYS		-38		/**< Function not implemented */
#define CSP_ERR_HMAC		-100		/**< HMAC failed */
#define CSP_ERR_CRC32		-102		/**< CRC32 failed */
#define CSP_ERR_SFP		-103		/**< SFP protocol error or inconsistency */
/**@}*/
