#ifndef CSP_IF_RF_H
#define CSP_IF_RF_H


#include <csp/csp_interface.h>


#define CSP_IF_RF_DEFAULT_NAME "RF"

/**
 * Send RF USLP frame.
 *
 * Used by csp_rf_tx() to send USLP frames.
 *
 * @param[in] data CSP packet
 * @return CSP_ERR_NONE on success, otherwise an error code.
 */
typedef int (*csp_rf_driver_tx_t)( csp_packet_t * data );

/**
 * Interface data (state information).
 */
typedef struct
{
	uint32_t            cfp_packet_counter; /**< CFP Identification number - same number on all fragments from same CSP packet. */
	csp_rf_driver_tx_t  tx_func;            /**< Tx function */
	csp_packet_t        *pbufs;             /**< PBUF queue */
} csp_rf_interface_data_t;

/**
 * Add interface.
 *
 * @param[in] iface CSP interface, initialized with name and inteface_data
 * 								pointing to a valid #csp_rf_interface_data_t structure.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_rf_add_interface(csp_iface_t * iface);

/**
 * Remove interface.
 *
 * @param[in] iface CSP interface to be removed.
 *
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_rf_remove_interface(csp_iface_t * iface);



#endif // CSP_IF_RF_H