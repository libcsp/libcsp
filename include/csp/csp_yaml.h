/*****************************************************************************
 * **File:** csp/csp_yaml.h
 *
 * **Description:** Configure CSP stack from YAML configuration file
 ****************************************************************************/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Setup the CSP stack based on a yaml configuration file
 *
 * @param[in] filename YAML configuration file.
 * @param[in] dfl_addr The default interface address can be overriden
 * 					   by passing a pointer to an integer higher than zero.
 * 					   The default interface address can be read back using the
 * 					   same integer, if its set to zero. Passing NULL to
 * 					   default address ignores the override, or readback.
 */
void csp_yaml_init(char * filename, unsigned int * dfl_addr);

#ifdef __cplusplus
}
#endif
