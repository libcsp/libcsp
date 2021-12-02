#pragma once

/**
 * Setup the CSP stack based on a yaml configuration file
 * 
 * @param dfl_addr:
 * The default interface address can be overriden by passing a pointer to an integer higher than zero
 * The default interface address can be read back using the same integer, if its set to zero
 * Passing NULL to default address ignores the override, or readback.
 * 
 */
void csp_yaml_init(char * filename, unsigned int * dfl_addr);