/****************************************************************************
 * **File:** csp/csp_traceroute.h
 *
 * **Description:** Traceroute support for CSP virtual topology simulation
 *
 * This header defines the traceroute API for the CSP library. Traceroute
 * functionality is designed exclusively for the virtual topology simulator
 * and is disabled by default in production builds.
 *
 * All traceroute code is conditionally compiled using #if CSP_TRACEROUTE
 * to ensure zero impact on production builds.
 ****************************************************************************/
#pragma once

#if CSP_TRACEROUTE

#include <csp/csp_types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Routing decision codes (success cases)
 */
#define CSP_TRACE_RECEIVED           0   /**< Packet received at node */
#define CSP_TRACE_ROUTE_LOOPBACK     1   /**< R01: Routed via loopback */
#define CSP_TRACE_ROUTE_SUBNET       2   /**< R02: Routed via subnet match */
#define CSP_TRACE_ROUTE_TABLE        3   /**< R03: Routed via routing table */
#define CSP_TRACE_ROUTE_DEFAULT      4   /**< R04: Routed via default interface */
#define CSP_TRACE_DELIVERED          5   /**< R05: Delivered to local application */

/**
 * Drop reason codes (failure cases)
 */
#define CSP_TRACE_DROP_DUPLICATE     10  /**< D01: Duplicate packet */
#define CSP_TRACE_DROP_UNSUP_HMAC    11  /**< D02: HMAC not supported */
#define CSP_TRACE_DROP_UNSUP_RDP     12  /**< D03: RDP not supported */
#define CSP_TRACE_DROP_CRC32_FAIL    13  /**< D04: CRC32 verification failed */
#define CSP_TRACE_DROP_CRC32_REQ     14  /**< D05: CRC32 required but missing */
#define CSP_TRACE_DROP_HMAC_FAIL     15  /**< D06: HMAC verification failed */
#define CSP_TRACE_DROP_HMAC_REQ      16  /**< D07: HMAC required but missing */
#define CSP_TRACE_DROP_RDP_REQ       17  /**< D08: RDP required but missing */
#define CSP_TRACE_DROP_NO_SOCKET     18  /**< D09: No socket listening */
#define CSP_TRACE_DROP_QUEUE_SOCKET  19  /**< D10: Socket queue full */
#define CSP_TRACE_DROP_QUEUE_CONN    20  /**< D11: Connection queue full */
#define CSP_TRACE_DROP_NO_CONN       21  /**< D12: No connection available */
#define CSP_TRACE_DROP_SOCK_QUEUE    22  /**< D13: Socket queue full (conn) */
#define CSP_TRACE_DROP_NO_ROUTE      23  /**< D14: No route found */
#define CSP_TRACE_DROP_SPLIT_HOR_SUB 24  /**< D15: Split-horizon (subnet, same iface) */
#define CSP_TRACE_DROP_SPLIT_HOR_SA  25  /**< D16: Split-horizon (subnet, addr check) */
#define CSP_TRACE_DROP_SPLIT_HOR_RT  26  /**< D17: Split-horizon (rtable, same iface) */
#define CSP_TRACE_DROP_SPLIT_HOR_RA  27  /**< D18: Split-horizon (rtable, addr check) */
#define CSP_TRACE_DROP_SPLIT_HOR_DEF 28  /**< D19: Split-horizon (default, same iface) */
#define CSP_TRACE_DROP_SPLIT_HOR_DA  29  /**< D20: Split-horizon (default, addr check) */
#define CSP_TRACE_DROP_CLONE_FAIL    30  /**< D21: Buffer clone failed */
#define CSP_TRACE_DROP_HMAC_APP_FAIL 31  /**< D22: HMAC append failed */
#define CSP_TRACE_DROP_HMAC_UNSUP    32  /**< D23: HMAC unsupported */
#define CSP_TRACE_DROP_CRC32_APP_FAIL 33 /**< D24: CRC32 append failed */
#define CSP_TRACE_DROP_IFACE_TX_FAIL 34  /**< D25: Interface TX failed */

/**
 * Trace hop data structure
 *
 * Contains information about a single hop in the packet's journey
 */
typedef struct {
    uint16_t node_addr;        /**< Address of this node */
    char iface_name[16];       /**< Interface name */
    uint32_t timestamp_ms;     /**< Timestamp when packet passed through */
    uint8_t action;            /**< 0=received, 1=forwarded, 2=dropped */
    uint8_t route_code;        /**< See CSP_TRACE_* constants above */
    uint16_t via;              /**< Via address for routing table entries (CSP_NO_VIA_ADDRESS if N/A) */
} csp_trace_hop_t;

/**
 * Callback function type for trace events
 *
 * This callback is invoked by the CSP library whenever a trace event occurs
 * (packet received, forwarded, or dropped). The simulator implements this
 * callback to collect and aggregate trace data.
 *
 * @param id Pointer to the CSP packet ID
 * @param hop Pointer to the trace hop data
 */
typedef void (*csp_trace_callback_t)(const csp_id_t *id, const csp_trace_hop_t *hop);

/**
 * Initialize traceroute support
 *
 * Registers a callback function that will be invoked for all trace events.
 * This function should be called during node initialization in the simulator.
 *
 * @param callback Function to call for each trace event (NULL to disable)
 */
void csp_traceroute_init(csp_trace_callback_t callback);

/**
 * Log a trace hop event
 *
 * This function is called internally by the CSP library at various routing
 * decision points. It should not be called directly by user code.
 *
 * @param id Pointer to the CSP packet ID
 * @param iface Pointer to the interface
 * @param action Action code (0=received, 1=forwarded, 2=dropped)
 * @param route_code Routing decision or drop reason code
 * @param via Via address (CSP_NO_VIA_ADDRESS if not applicable)
 */
void csp_traceroute_log_hop(const csp_id_t *id, csp_iface_t *iface, uint8_t action, uint8_t route_code, uint16_t via);

#ifdef __cplusplus
}
#endif

#endif /* CSP_TRACEROUTE */

