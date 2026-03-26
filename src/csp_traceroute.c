/****************************************************************************
 * **File:** csp_traceroute.c
 *
 * **Description:** Traceroute implementation for CSP virtual topology simulation
 ****************************************************************************/

#include <csp/csp.h>
#include <csp/csp_traceroute.h>

#if CSP_TRACEROUTE
#include <csp/arch/csp_time.h>
#include <string.h>
#include <stdio.h>

/**
 * Global trace callback function pointer
 *
 * This is set by csp_traceroute_init() and invoked whenever a trace event occurs.
 * NULL if traceroute is not initialized.
 */
static csp_trace_callback_t trace_callback = NULL;

void csp_traceroute_init(csp_trace_callback_t callback) {
    trace_callback = callback;
}

void csp_traceroute_log_hop(const csp_id_t *id, csp_iface_t *iface, uint8_t action, uint8_t route_code, uint16_t via) {
    /* Do nothing if callback is not registered */
    if (trace_callback == NULL) {
        return;
    }

    /* Do nothing if interface is NULL */
    if (iface == NULL) {
        return;
    }

    /* Build trace hop structure */
    csp_trace_hop_t hop;
    hop.node_addr = iface->addr;
    strncpy(hop.iface_name, iface->name, sizeof(hop.iface_name) - 1);
    hop.iface_name[sizeof(hop.iface_name) - 1] = '\0';
    hop.timestamp_ms = csp_get_ms();
    hop.action = action;
    hop.route_code = route_code;
    hop.via = via;

    /* Invoke callback */
    trace_callback(id, &hop);
}

#endif /* CSP_TRACEROUTE */

