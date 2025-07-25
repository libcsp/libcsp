#ifndef LIBCSP_GRLIBCAN_H
#define LIBCSP_GRLIBCAN_H
/* Include libCSP */
#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

/* System includes */
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <csp/csp_debug.h>
#include <stdint.h>

/* Driver core include */
#include "grlib-can-core.h"

/* CAN interface data - socket CAN copy */
typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;
	csp_can_interface_data_t ifdata;
	pthread_t rx_thread;
	grlibCan_dev_t * device;
	bool kill;
} can_context_t;

#define LIBCSP_STACK_POOL 2
#define LIBCSP_STACK_SZ   4096

extern int stack_ptr;
extern char stacks[LIBCSP_STACK_POOL][LIBCSP_STACK_SZ];

void grlibCan_free(can_context_t * ctx);

void grlibCan_rx_thread(void * arg);

int csp_can_grlibCan_tx_frame(void * driver_data, uint32_t id,
							  const uint8_t * data, uint8_t dlc);

int csp_can_grlibCan_set_promisc(const bool promisc, can_context_t * ctx);

int csp_can_grlibCan_open_and_add_interface(can_context_t * ctx,
											grlibCan_dev_t * device,
											const char * ifname,
											unsigned int node_id,
											int baudrate, bool promisc,
											csp_iface_t ** return_iface);

csp_iface_t * csp_can_grlibCan_init(can_context_t * ctx, grlibCan_dev_t * device,
									unsigned int node_id,
									int baudrate, bool promisc);

int csp_can_grlibCan_stop(csp_iface_t * iface);

#endif
