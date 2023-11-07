/****************************************************************************
 * **File:** csp/arch/csp_time.h
 *
 * **Description:** CSP time
 ****************************************************************************/
#pragma once

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t csp_get_ms(void);
uint32_t csp_get_ms_isr(void);
uint32_t csp_get_s(void);
uint32_t csp_get_s_isr(void);

#ifdef __cplusplus
}
#endif
