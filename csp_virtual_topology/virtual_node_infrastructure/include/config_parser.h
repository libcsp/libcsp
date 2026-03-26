/**
 * @file config_parser.h
 * @brief JSON configuration parser for CSP virtual topology
 *
 * This module provides functions to parse JSON topology configuration files
 * and extract node-specific configuration for CSP virtual nodes.
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ZMQ proxy configuration
 */
typedef struct {
    char *name;              /**< Proxy name (e.g., "RF", "PRIMARY_CAN") */
    char *description;       /**< Proxy description */
    char *host;              /**< Proxy host (default: "localhost") */
    int subscribe_port;      /**< Subscriber port */
    int publish_port;        /**< Publisher port */
    uint16_t subnet_prefix;  /**< Subnet prefix (network portion of address) */
    uint8_t netmask;         /**< Subnet netmask (number of host bits) */
} zmq_proxy_config_t;

/**
 * @brief Interface configuration
 */
typedef struct {
    char *name;              /**< Interface name (e.g., "ZMQ0", "CAN0") */
    char *driver;            /**< Driver type (e.g., "zmq") */
    char *zmq_proxy;         /**< ZMQ proxy name this interface connects to */
    uint16_t address;        /**< CSP address on this interface */
    uint8_t netmask;         /**< CSP netmask */
    bool is_default;         /**< Is this the default interface */
} interface_config_t;

/**
 * @brief Routing table entry
 */
typedef struct {
    uint16_t address;        /**< Destination address */
    uint8_t netmask;         /**< Netmask */
    char *interface;         /**< Interface name */
    int16_t via;             /**< Via address (-1 if direct) */
    char *comment;           /**< Optional comment */
} routing_entry_t;

/**
 * @brief Node configuration
 */
typedef struct {
    char *name;              /**< Node name */
    char *description;       /**< Node description */

    interface_config_t *interfaces;  /**< Array of interfaces */
    int num_interfaces;              /**< Number of interfaces */

    routing_entry_t *routing_table;  /**< Array of routing entries */
    int num_routes;                  /**< Number of routing entries */

    bool enable_ping;        /**< Enable ping service */
    bool enable_echo;        /**< Enable echo service */
    int custom_port;         /**< Custom application port (-1 if not set) */
} node_config_t;

/**
 * @brief Monitoring configuration
 */
typedef struct {
    bool enabled;            /**< Monitoring enabled */
    char *log_file;          /**< Log file path */
    char *log_format;        /**< Log format (text, json, csv) */
    bool promiscuous_mode;   /**< Enable promiscuous mode */
    bool track_routing_path; /**< Track routing paths */
    bool detect_duplicates;  /**< Detect duplicate packets */
    int statistics_interval; /**< Statistics interval in seconds */
} monitoring_config_t;

/**
 * @brief Complete topology configuration
 */
typedef struct {
    char *name;              /**< Topology name */
    char *description;       /**< Topology description */
    int csp_version;         /**< CSP version (1 or 2) */
    char *deduplication;     /**< Deduplication mode (all, off) */

    zmq_proxy_config_t *zmq_proxies;  /**< Array of ZMQ proxies */
    int num_proxies;                  /**< Number of ZMQ proxies */

    node_config_t *nodes;    /**< Array of nodes */
    int num_nodes;           /**< Number of nodes */

    monitoring_config_t monitoring;   /**< Monitoring configuration */
} topology_config_t;

/**
 * @brief Parse topology configuration from JSON file
 *
 * @param filename Path to JSON configuration file
 * @return Pointer to topology configuration, or NULL on error
 */
topology_config_t* parse_topology_config(const char* filename);

/**
 * @brief Get configuration for a specific node by name
 *
 * @param topology Topology configuration
 * @param node_name Name of the node to find
 * @return Pointer to node configuration, or NULL if not found
 */
node_config_t* get_node_config(topology_config_t* topology, const char* node_name);

/**
 * @brief Generate CSP routing table string from node configuration
 *
 * @param node Node configuration
 * @return Dynamically allocated routing table string (caller must free), or NULL on error
 */
char* generate_rtable_string(node_config_t* node);

/**
 * @brief Validate topology configuration
 *
 * @param topology Topology configuration to validate
 * @return 0 on success, -1 on validation error
 */
int validate_topology_config(topology_config_t* topology);

/**
 * @brief Free topology configuration and all associated memory
 *
 * @param topology Topology configuration to free
 */
void free_topology_config(topology_config_t* topology);

/**
 * @brief Free node configuration and all associated memory
 *
 * @param node Node configuration to free
 */
void free_node_config(node_config_t* node);

/**
 * @brief Find ZMQ proxy configuration by name
 *
 * @param topology Topology configuration
 * @param name Proxy name to find
 * @return Pointer to proxy configuration, or NULL if not found
 */
zmq_proxy_config_t* find_proxy_by_name(topology_config_t* topology, const char* name);

/**
 * @brief Calculate fully-qualified 14-bit CSP address
 *
 * Combines subnet prefix with host address using the formula:
 * full_address = (subnet_prefix << (14 - netmask)) | host_address
 *
 * @param subnet_prefix Subnet prefix (network portion of address)
 * @param netmask Number of host bits
 * @param host_address Host portion of the address
 * @return Full 14-bit CSP address
 */
uint16_t calculate_full_csp_address(uint16_t subnet_prefix, uint8_t netmask, uint16_t host_address);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_PARSER_H */

