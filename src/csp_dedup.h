/*
 * csp_dedup.h
 *
 *  Created on: 04/11/2014
 *      Author: johan
 */

#ifndef CSP_DEDUP_H_
#define CSP_DEDUP_H_

/**
 * Check for a duplicate packet
 * @param packet pointer to packet
 * @return 0 if not a duplicate, 1 if it IS a duplicate
 */
int csp_dedup_check(csp_packet_t * packet);

#endif /* CSP_DEDUP_H_ */
