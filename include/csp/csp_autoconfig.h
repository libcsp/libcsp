/*
 * csp_autoconfig.h
 *
 *  Created on: Sep 8, 2016
 *      Author: johan
 */

#ifndef LIBCSP_INCLUDE_CSP_CSP_AUTOCONFIG_H_
#define LIBCSP_INCLUDE_CSP_CSP_AUTOCONFIG_H_

#include <csp/csp.h>

#define GIT_REV "1234"
#define CSP_FREERTOS 1
#define CSP_DEBUG 1
#define CSP_USE_RDP
#define CSP_USE_CRC32
#undef CSP_USE_HMAC
#undef CSP_USE_XTEA
#undef CSP_USE_PROMISC

#undef CSP_USE_QOS
#undef CSP_BUFFER_STATIC

#define CSP_CONNECTION_SO CSP_SO_NONE

#define CSP_BUFFER_COUNT 12
#define CSP_BUFFER_SIZE 320
#define CSP_CONN_MAX 4
#define CSP_CONN_QUEUE_LENGTH 10
#define CSP_FIFO_INPUT 10
#define CSP_MAX_BIND_PORT 31
#define CSP_RDP_MAX_WINDOW 20
#define CSP_PADDING_BYTES 8
#define CSP_LITTLE_ENDIAN 1

#define FREERTOS_VERSION 8
#define CSP_HAVE_STDBOOL_H

#define CSP_DEBUG
#define CSP_LOG_LEVEL_DEBUG 1

#endif /* LIBCSP_INCLUDE_CSP_CSP_AUTOCONFIG_H_ */
