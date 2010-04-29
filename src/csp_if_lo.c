/*
 * csp_if_lo.c
 *
 *  Created on: 15-09-2009
 *      Author: Administrator
 */

/* CSP includes */
#include <csp/csp.h>

//#include <FreeRTOS.h>

int csp_lo_tx(csp_id_t idout, csp_packet_t * packet, unsigned int timeout) {

	/* Store outgoing id */
	packet->id.ext = idout.ext;

	/* Send back into CSP */

    /* We need a portable interface for ISRs ! */
	//portENTER_CRITICAL(); 
	
    csp_new_packet(packet, csp_lo_tx, NULL);	

    //portEXIT_CRITICAL();

	return 1;

}
