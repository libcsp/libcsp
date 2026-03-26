/**
 * @file config_parser.c
 * @brief JSON configuration parser implementation
 */

#include "config_parser.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple logging macro (replace with csp_print when csp_es is available) */
#define csp_print printf

/* Helper function to safely get string from JSON object */
static char* get_json_string(cJSON *obj, const char *key, const char *default_val) {
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsString(item)) {
        return strdup(item->valuestring);
    }
    return default_val ? strdup(default_val) : NULL;
}

/* Helper function to safely get integer from JSON object */
static int get_json_int(cJSON *obj, const char *key, int default_val) {
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsNumber(item)) {
        return item->valueint;
    }
    return default_val;
}

/* Helper function to safely get boolean from JSON object */
static bool get_json_bool(cJSON *obj, const char *key, bool default_val) {
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }
    return default_val;
}

/* Parse ZMQ proxy configuration */
static zmq_proxy_config_t* parse_zmq_proxy(cJSON *proxy_obj) {
    zmq_proxy_config_t *proxy = calloc(1, sizeof(zmq_proxy_config_t));
    if (!proxy) return NULL;

    proxy->name = get_json_string(proxy_obj, "name", "default");
    proxy->description = get_json_string(proxy_obj, "description", "");
    proxy->host = get_json_string(proxy_obj, "host", "localhost");
    proxy->subscribe_port = get_json_int(proxy_obj, "subscribe_port", 6000);
    proxy->publish_port = get_json_int(proxy_obj, "publish_port", 7000);
    proxy->subnet_prefix = (uint16_t)get_json_int(proxy_obj, "subnet_prefix", 0);
    proxy->netmask = (uint8_t)get_json_int(proxy_obj, "netmask", 8);

    return proxy;
}

/* Find ZMQ proxy by name */
zmq_proxy_config_t* find_proxy_by_name(topology_config_t* topology, const char* name) {
    if (!topology || !name) return NULL;

    for (int i = 0; i < topology->num_proxies; i++) {
        if (topology->zmq_proxies[i].name && strcmp(topology->zmq_proxies[i].name, name) == 0) {
            return &topology->zmq_proxies[i];
        }
    }
    return NULL;
}

/* Calculate fully-qualified 14-bit CSP address */
uint16_t calculate_full_csp_address(uint16_t subnet_prefix, uint8_t netmask, uint16_t host_address) {
    /* CSP v2 uses 14-bit addresses
     * netmask = number of host bits
     * network bits = 14 - netmask
     * full_address = (subnet_prefix << (14 - netmask)) | (host_address & ((1 << netmask) - 1))
     */
    uint8_t network_bits = 14 - netmask;
    uint16_t host_mask = (1 << netmask) - 1;
    uint16_t full_address = ((subnet_prefix << network_bits) | (host_address & host_mask)) & 0x3FFF;
    return full_address;
}

/* Parse interface configuration */
static interface_config_t* parse_interface(cJSON *if_obj) {
    interface_config_t *iface = calloc(1, sizeof(interface_config_t));
    if (!iface) return NULL;

    iface->name = get_json_string(if_obj, "name", "ZMQ0");
    iface->driver = get_json_string(if_obj, "driver", "zmq");
    iface->zmq_proxy = get_json_string(if_obj, "zmq_proxy", NULL);
    iface->address = (uint16_t)get_json_int(if_obj, "address", 0);
    iface->netmask = (uint8_t)get_json_int(if_obj, "netmask", 8);
    iface->is_default = get_json_bool(if_obj, "is_default", false);

    return iface;
}

/* Parse routing table entry */
static routing_entry_t* parse_routing_entry(cJSON *route_obj) {
    routing_entry_t *route = calloc(1, sizeof(routing_entry_t));
    if (!route) return NULL;

    route->address = (uint16_t)get_json_int(route_obj, "address", 0);
    route->netmask = (uint8_t)get_json_int(route_obj, "netmask", 0);
    route->interface = get_json_string(route_obj, "interface", "ZMQ0");

    cJSON *via = cJSON_GetObjectItem(route_obj, "via");
    if (via && cJSON_IsNumber(via)) {
        route->via = (int16_t)via->valueint;
    } else {
        route->via = -1;  /* Direct route */
    }

    route->comment = get_json_string(route_obj, "comment", NULL);

    return route;
}

/* Parse node configuration */
static node_config_t* parse_node(cJSON *node_obj) {
    node_config_t *node = calloc(1, sizeof(node_config_t));
    if (!node) return NULL;

    node->name = get_json_string(node_obj, "name", "unnamed");
    node->description = get_json_string(node_obj, "description", "");

    /* Parse interfaces */
    cJSON *interfaces = cJSON_GetObjectItem(node_obj, "interfaces");
    if (interfaces && cJSON_IsArray(interfaces)) {
        node->num_interfaces = cJSON_GetArraySize(interfaces);
        node->interfaces = calloc(node->num_interfaces, sizeof(interface_config_t));

        for (int i = 0; i < node->num_interfaces; i++) {
            cJSON *if_obj = cJSON_GetArrayItem(interfaces, i);
            interface_config_t *iface = parse_interface(if_obj);
            if (iface) {
                node->interfaces[i] = *iface;
                free(iface);
            }
        }
    }

    /* Parse routing table - support both "routing_table" and "routes" field names */
    cJSON *routing_table = cJSON_GetObjectItem(node_obj, "routing_table");
    if (!routing_table) {
        routing_table = cJSON_GetObjectItem(node_obj, "routes");
    }

    if (routing_table && cJSON_IsArray(routing_table)) {
        node->num_routes = cJSON_GetArraySize(routing_table);
        node->routing_table = calloc(node->num_routes, sizeof(routing_entry_t));

        for (int i = 0; i < node->num_routes; i++) {
            cJSON *route_obj = cJSON_GetArrayItem(routing_table, i);
            routing_entry_t *route = parse_routing_entry(route_obj);
            if (route) {
                node->routing_table[i] = *route;
                free(route);
            }
        }
    }

    /* Parse services */
    cJSON *services = cJSON_GetObjectItem(node_obj, "services");
    if (services) {
        node->enable_ping = get_json_bool(services, "ping", true);
        node->enable_echo = get_json_bool(services, "echo", true);
        node->custom_port = get_json_int(services, "custom_port", -1);
    } else {
        node->enable_ping = true;
        node->enable_echo = true;
        node->custom_port = -1;
    }

    return node;
}

/* Parse monitoring configuration */
static void parse_monitoring(cJSON *mon_obj, monitoring_config_t *monitoring) {
    if (!mon_obj || !monitoring) return;

    monitoring->enabled = get_json_bool(mon_obj, "enabled", true);
    monitoring->log_file = get_json_string(mon_obj, "log_file", "topology.log");
    monitoring->log_format = get_json_string(mon_obj, "log_format", "text");
    monitoring->promiscuous_mode = get_json_bool(mon_obj, "promiscuous_mode", true);
    monitoring->track_routing_path = get_json_bool(mon_obj, "track_routing_path", true);
    monitoring->detect_duplicates = get_json_bool(mon_obj, "detect_duplicates", true);
    monitoring->statistics_interval = get_json_int(mon_obj, "statistics_interval", 5);
}

/* Main parsing function */
topology_config_t* parse_topology_config(const char* filename) {
    if (!filename) {
        csp_print("ERROR: NULL filename provided\n");
        return NULL;
    }

    /* Read file */
    FILE *file = fopen(filename, "r");
    if (!file) {
        csp_print("ERROR: Failed to open config file: %s\n", filename);
        return NULL;
    }

    /* Get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Read file content */
    char *json_str = malloc(file_size + 1);
    if (!json_str) {
        fclose(file);
        csp_print("ERROR: Failed to allocate memory for JSON\n");
        return NULL;
    }

    size_t read_size = fread(json_str, 1, file_size, file);
    json_str[read_size] = '\0';
    fclose(file);

    /* Parse JSON */
    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr) {
            csp_print("ERROR: JSON parse error before: %s\n", error_ptr);
        }
        return NULL;
    }

    /* Allocate topology config */
    topology_config_t *topology = calloc(1, sizeof(topology_config_t));
    if (!topology) {
        cJSON_Delete(root);
        return NULL;
    }

    /* Parse topology section */
    cJSON *topo_obj = cJSON_GetObjectItem(root, "topology");
    if (topo_obj) {
        topology->name = get_json_string(topo_obj, "name", "unnamed");
        topology->description = get_json_string(topo_obj, "description", "");
        topology->csp_version = get_json_int(topo_obj, "csp_version", 2);
        topology->deduplication = get_json_string(topo_obj, "deduplication", "all");

        /* Parse ZMQ proxies */
        cJSON *proxies = cJSON_GetObjectItem(topo_obj, "zmq_proxies");
        if (proxies && cJSON_IsArray(proxies)) {
            topology->num_proxies = cJSON_GetArraySize(proxies);
            topology->zmq_proxies = calloc(topology->num_proxies, sizeof(zmq_proxy_config_t));

            for (int i = 0; i < topology->num_proxies; i++) {
                cJSON *proxy_obj = cJSON_GetArrayItem(proxies, i);
                zmq_proxy_config_t *proxy = parse_zmq_proxy(proxy_obj);
                if (proxy) {
                    topology->zmq_proxies[i] = *proxy;
                    free(proxy);
                }
            }
        } else {
            /* Single default proxy */
            topology->num_proxies = 1;
            topology->zmq_proxies = calloc(1, sizeof(zmq_proxy_config_t));
            topology->zmq_proxies[0].name = strdup("default");
            topology->zmq_proxies[0].host = get_json_string(topo_obj, "host", "localhost");
            topology->zmq_proxies[0].subscribe_port = get_json_int(topo_obj, "subscribe_port", 6000);
            topology->zmq_proxies[0].publish_port = get_json_int(topo_obj, "publish_port", 7000);
        }
    }

    /* Parse nodes */
    cJSON *nodes = cJSON_GetObjectItem(root, "nodes");
    if (nodes && cJSON_IsArray(nodes)) {
        topology->num_nodes = cJSON_GetArraySize(nodes);
        topology->nodes = calloc(topology->num_nodes, sizeof(node_config_t));

        for (int i = 0; i < topology->num_nodes; i++) {
            cJSON *node_obj = cJSON_GetArrayItem(nodes, i);
            node_config_t *node = parse_node(node_obj);
            if (node) {
                topology->nodes[i] = *node;
                free(node);
            }
        }
    }

    /* Parse monitoring */
    cJSON *monitoring = cJSON_GetObjectItem(root, "monitoring");
    if (monitoring) {
        parse_monitoring(monitoring, &topology->monitoring);
    }

    cJSON_Delete(root);

    /* Compute fully-qualified addresses for all interfaces */
    for (int i = 0; i < topology->num_nodes; i++) {
        node_config_t *node = &topology->nodes[i];
        for (int j = 0; j < node->num_interfaces; j++) {
            interface_config_t *iface = &node->interfaces[j];
            if (iface->zmq_proxy) {
                zmq_proxy_config_t *proxy = find_proxy_by_name(topology, iface->zmq_proxy);
                if (proxy) {
                    uint16_t host_addr = iface->address;
                    iface->address = calculate_full_csp_address(proxy->subnet_prefix, proxy->netmask, host_addr);
                    /* Also sync netmask from proxy */
                    iface->netmask = proxy->netmask;

                    csp_print("[CONFIG] Node '%s', Interface '%s': host_addr=%u -> full_addr=%u (subnet_prefix=%u, netmask=%u)\n",
                              node->name, iface->name, host_addr, iface->address, proxy->subnet_prefix, proxy->netmask);
                }
            }
        }
    }

    csp_print("Parsed topology '%s' with %d nodes and %d ZMQ proxies\n",
              topology->name, topology->num_nodes, topology->num_proxies);

    return topology;
}

/* Get node configuration by name */
node_config_t* get_node_config(topology_config_t* topology, const char* node_name) {
    if (!topology || !node_name) {
        return NULL;
    }

    for (int i = 0; i < topology->num_nodes; i++) {
        if (topology->nodes[i].name && strcmp(topology->nodes[i].name, node_name) == 0) {
            return &topology->nodes[i];
        }
    }

    csp_print("ERROR: Node '%s' not found in topology\n", node_name);
    return NULL;
}

/* Generate CSP routing table string */
char* generate_rtable_string(node_config_t* node) {
    if (!node || node->num_routes == 0) {
        return NULL;
    }

    /* Calculate required buffer size */
    size_t buffer_size = 0;
    for (int i = 0; i < node->num_routes; i++) {
        /* Format: "address/netmask INTERFACE [via], " */
        buffer_size += 64;  /* Conservative estimate per route */
    }

    char *rtable_str = malloc(buffer_size);
    if (!rtable_str) {
        return NULL;
    }

    rtable_str[0] = '\0';

    for (int i = 0; i < node->num_routes; i++) {
        routing_entry_t *route = &node->routing_table[i];
        char route_str[64];

        if (route->via >= 0) {
            /* Route with via address */
            snprintf(route_str, sizeof(route_str), "%u/%u %s %d",
                     route->address, route->netmask, route->interface, route->via);
        } else {
            /* Direct route */
            snprintf(route_str, sizeof(route_str), "%u/%u %s",
                     route->address, route->netmask, route->interface);
        }

        strcat(rtable_str, route_str);

        /* Add comma separator except for last entry */
        if (i < node->num_routes - 1) {
            strcat(rtable_str, ", ");
        }
    }

    return rtable_str;
}

/* Validate topology configuration */
int validate_topology_config(topology_config_t* topology) {
    if (!topology) {
        csp_print("ERROR: NULL topology\n");
        return -1;
    }

    if (topology->num_nodes == 0) {
        csp_print("ERROR: No nodes defined in topology\n");
        return -1;
    }

    /* Check for address conflicts within the same subnet (zmq_proxy) */
    for (int i = 0; i < topology->num_nodes; i++) {
        node_config_t *node1 = &topology->nodes[i];
        for (int j = 0; j < node1->num_interfaces; j++) {
            uint16_t addr1 = node1->interfaces[j].address;
            const char *proxy1 = node1->interfaces[j].zmq_proxy;

            /* Check against other nodes */
            for (int k = i + 1; k < topology->num_nodes; k++) {
                node_config_t *node2 = &topology->nodes[k];
                for (int l = 0; l < node2->num_interfaces; l++) {
                    uint16_t addr2 = node2->interfaces[l].address;
                    const char *proxy2 = node2->interfaces[l].zmq_proxy;

                    /* Only conflict if same address AND same subnet */
                    if (addr1 == addr2 && proxy1 && proxy2 && strcmp(proxy1, proxy2) == 0) {
                        csp_print("ERROR: Address conflict: nodes '%s' and '%s' both use address %u on subnet '%s'\n",
                                  node1->name, node2->name, addr1, proxy1);
                        return -1;
                    }
                }
            }
        }
    }

    /* Validate routing table references */
    for (int i = 0; i < topology->num_nodes; i++) {
        node_config_t *node = &topology->nodes[i];

        for (int j = 0; j < node->num_routes; j++) {
            routing_entry_t *route = &node->routing_table[j];

            /* Check if interface exists */
            bool found = false;
            for (int k = 0; k < node->num_interfaces; k++) {
                if (strcmp(node->interfaces[k].name, route->interface) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                csp_print("ERROR: Node '%s' routing entry references non-existent interface '%s'\n",
                          node->name, route->interface);
                return -1;
            }

            /* Validate netmask */
            if (route->netmask > 16) {
                csp_print("ERROR: Node '%s' has invalid netmask %u (max 16)\n",
                          node->name, route->netmask);
                return -1;
            }
        }
    }

    csp_print("Topology validation passed\n");
    return 0;
}

/* Free node configuration */
void free_node_config(node_config_t* node) {
    if (!node) return;

    free(node->name);
    free(node->description);

    /* Free interfaces */
    for (int i = 0; i < node->num_interfaces; i++) {
        free(node->interfaces[i].name);
        free(node->interfaces[i].driver);
        free(node->interfaces[i].zmq_proxy);
    }
    free(node->interfaces);

    /* Free routing table */
    for (int i = 0; i < node->num_routes; i++) {
        free(node->routing_table[i].interface);
        free(node->routing_table[i].comment);
    }
    free(node->routing_table);
}

/* Free topology configuration */
void free_topology_config(topology_config_t* topology) {
    if (!topology) return;

    free(topology->name);
    free(topology->description);
    free(topology->deduplication);

    /* Free ZMQ proxies */
    for (int i = 0; i < topology->num_proxies; i++) {
        free(topology->zmq_proxies[i].name);
        free(topology->zmq_proxies[i].description);
        free(topology->zmq_proxies[i].host);
    }
    free(topology->zmq_proxies);

    /* Free nodes */
    for (int i = 0; i < topology->num_nodes; i++) {
        free_node_config(&topology->nodes[i]);
    }
    free(topology->nodes);

    /* Free monitoring */
    free(topology->monitoring.log_file);
    free(topology->monitoring.log_format);

    free(topology);
}

