/*
 * csp_id.h
 *
 *  Created on: 30. jun. 2020
 *      Author: johan
 */

#ifndef LIB_CSP_SRC_CSP_ID_H_
#define LIB_CSP_SRC_CSP_ID_H_

void csp_id1_prepend(csp_packet_t * packet);
int csp_id1_strip(csp_packet_t * packet);
void csp_id1_setup_rx(csp_packet_t * packet);

void csp_id2_prepend(csp_packet_t * packet);
int csp_id2_strip(csp_packet_t * packet);
void csp_id2_setup_rx(csp_packet_t * packet);

void csp_id_prepend(csp_packet_t * packet);
int csp_id_strip(csp_packet_t * packet);
int csp_id_setup_rx(csp_packet_t * packet);
unsigned int csp_id_get_host_bits(void);
unsigned int csp_id_get_max_nodeid(void);
unsigned int csp_id_get_max_port(void);


#endif /* LIB_CSP_SRC_CSP_ID_H_ */
