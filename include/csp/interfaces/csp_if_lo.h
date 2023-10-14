/****************************************************************************
 * **File:** csp/interfaces/csp_if_lo.h
 *
 * **Description:** Loopback interface.
 ****************************************************************************/
#pragma once

#include <csp/csp_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Name of loopback interface.
 */
#define CSP_IF_LOOPBACK_NAME "LOOP"

/**
 *  Loopback interface.
 */
extern csp_iface_t csp_if_lo;

#ifdef __cplusplus
}
#endif
