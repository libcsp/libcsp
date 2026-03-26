/**
 * @file csp_virtual_node.c
 * @brief CSP Virtual Node - Configurable CSP node for topology simulation
 */

#include "config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#ifdef __GLIBC__
#include <execinfo.h>
#endif
#include <getopt.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#ifdef CSP_ES_AVAILABLE
#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/csp_iflist.h>
#include <csp/csp_rtable.h>
#include <csp/csp_promisc.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include <csp/arch/csp_time.h>
#if CSP_TRACEROUTE
#include <csp/csp_traceroute.h>
#endif
#include "../../../src/csp_conn.h"
#include <pthread.h>
#include <zmq.h>
#include <cJSON.h>
#endif

/* Global flag for graceful shutdown */
static volatile int running = 1;

/* Global node name for logging and control */
static char *node_name = NULL;
static FILE *log_file = NULL;

#if defined(CSP_ES_AVAILABLE) && CSP_TRACEROUTE
/* Global ZMQ socket for trace events */
static void *trace_zmq_ctx = NULL;
static void *trace_zmq_socket = NULL;

/**
 * Traceroute callback - sends trace events via ZMQ PUSH socket
 */
static void trace_callback_zmq(const csp_id_t *id, const csp_trace_hop_t *hop) {
    if (log_file) {
        fprintf(log_file, "[TRACE_CALLBACK] Called! socket=%p, id=%p, hop=%p\n",
                (void*)trace_zmq_socket, (void*)id, (void*)hop);
        fflush(log_file);
    }
    if (trace_zmq_socket == NULL || id == NULL || hop == NULL) {
        if (log_file) {
            fprintf(log_file, "[TRACE_CALLBACK] Early return due to NULL\n");
            fflush(log_file);
        }
        return;
    }

    /* Build JSON trace event */
    cJSON *event = cJSON_CreateObject();
    cJSON_AddStringToObject(event, "type", "trace");
    cJSON_AddNumberToObject(event, "src", id->src);
    cJSON_AddNumberToObject(event, "dst", id->dst);
    cJSON_AddNumberToObject(event, "dport", id->dport);
    cJSON_AddNumberToObject(event, "sport", id->sport);
    cJSON_AddNumberToObject(event, "node_addr", hop->node_addr);
    cJSON_AddStringToObject(event, "iface", hop->iface_name);
    cJSON_AddNumberToObject(event, "timestamp", hop->timestamp_ms);
    cJSON_AddNumberToObject(event, "action", hop->action);
    cJSON_AddNumberToObject(event, "route_code", hop->route_code);
    cJSON_AddNumberToObject(event, "via", hop->via);

    char *json_str = cJSON_PrintUnformatted(event);
    if (json_str) {
        int rc = zmq_send(trace_zmq_socket, json_str, strlen(json_str), ZMQ_DONTWAIT);
        if (log_file) {
            fprintf(log_file, "[TRACE] zmq_send returned %d (errno=%d), event: %s\n", rc, (rc < 0 ? zmq_errno() : 0), json_str);
            fflush(log_file);
        }
        free(json_str);
    }
    cJSON_Delete(event);
}
#endif

/* Signal handler for graceful shutdown */
static void signal_handler(int signum) {
    (void)signum;
    printf("\nReceived shutdown signal, cleaning up...\n");
    running = 0;
}

/* Crash handler for fatal signals - logs backtrace before dying */
static void crash_handler(int signum) {
    const char *signal_name;
    switch (signum) {
        case SIGSEGV: signal_name = "SIGSEGV (Segmentation fault)"; break;
        case SIGABRT: signal_name = "SIGABRT (Abort)"; break;
        case SIGFPE:  signal_name = "SIGFPE (Floating point exception)"; break;
        case SIGBUS:  signal_name = "SIGBUS (Bus error)"; break;
        default:      signal_name = "Unknown signal"; break;
    }

    /* Write to stderr immediately */
    fprintf(stderr, "\n[CRASH] Node '%s' received %s (signal %d)\n",
            node_name ? node_name : "unknown", signal_name, signum);

    /* Log to file if available */
    if (log_file) {
        fprintf(log_file, "\n[CRASH] ==========================================\n");
        fprintf(log_file, "[CRASH] FATAL: Received %s (signal %d)\n", signal_name, signum);
        fprintf(log_file, "[CRASH] Node: %s\n", node_name ? node_name : "unknown");

#ifdef __GLIBC__
        /* Get backtrace */
        void *bt_buffer[64];
        int bt_size = backtrace(bt_buffer, 64);
        fprintf(log_file, "[CRASH] Backtrace (%d frames):\n", bt_size);

        char **bt_symbols = backtrace_symbols(bt_buffer, bt_size);
        if (bt_symbols) {
            for (int i = 0; i < bt_size; i++) {
                fprintf(log_file, "[CRASH]   [%d] %s\n", i, bt_symbols[i]);
            }
            free(bt_symbols);
        } else {
            /* Fallback: write addresses directly */
            backtrace_symbols_fd(bt_buffer, bt_size, fileno(log_file));
        }
#else
        fprintf(log_file, "[CRASH] Backtrace not available (non-glibc system)\n");
#endif

        fprintf(log_file, "[CRASH] ==========================================\n");
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }

    /* Reset to default handler and re-raise to get proper exit status */
    signal(signum, SIG_DFL);
    raise(signum);
}

/* Helper function to sanitize topology name for use as directory name */
static void sanitize_dirname(const char *input, char *output, size_t output_size) {
    size_t i, j = 0;

    for (i = 0; input[i] != '\0' && j < output_size - 1; i++) {
        char c = input[i];

        /* Replace spaces with underscores */
        if (c == ' ') {
            output[j++] = '_';
        }
        /* Keep alphanumeric, dash, and underscore */
        else if (isalnum(c) || c == '-' || c == '_') {
            output[j++] = c;
        }
        /* Skip other characters */
    }

    output[j] = '\0';
}

#ifdef CSP_ES_AVAILABLE
/* Helper function to log CSP statistics */
static void log_csp_stats(void) {
    if (!log_file) {
        return;
    }

    uint32_t timestamp = csp_get_ms();

    fprintf(log_file, "\n[STATS] ========== CSP Statistics at %u ms ==========\n", timestamp);

    /* Log buffer statistics */
    int free_buffers = csp_buffer_remaining();
    fprintf(log_file, "[STATS] Buffers: %d free\n", free_buffers);

    /* Log interface statistics */
    csp_iface_t *iface = csp_iflist_get();
    int iface_count = 0;
    while (iface) {
        fprintf(log_file, "[STATS] Interface[%d] %s:\n", iface_count, iface->name);
        fprintf(log_file, "[STATS]   Address: %u, Netmask: %u, Default: %s\n",
                iface->addr, iface->netmask, iface->is_default ? "yes" : "no");
        fprintf(log_file, "[STATS]   TX: packets=%u bytes=%u errors=%u\n",
                iface->tx, iface->txbytes, iface->tx_error);
        fprintf(log_file, "[STATS]   RX: packets=%u bytes=%u errors=%u\n",
                iface->rx, iface->rxbytes, iface->rx_error);
        fprintf(log_file, "[STATS]   Drop: %u, Auth errors: %u, Frame errors: %u\n",
                iface->drop, iface->autherr, iface->frame);
        iface = iface->next;
        iface_count++;
    }

    /* Log connection statistics */
    size_t conn_array_size = 0;
    const csp_conn_t *conn_array = csp_conn_get_array(&conn_array_size);
    int active_conns = 0;

    for (size_t i = 0; i < conn_array_size; i++) {
        if (conn_array[i].state != CONN_CLOSED) {
            active_conns++;
        }
    }

    fprintf(log_file, "[STATS] Connections: %d active (out of %zu total)\n",
            active_conns, conn_array_size);

    fprintf(log_file, "[STATS] ==========================================\n\n");
    fflush(log_file);
}

/* CSP router task - forwards packets between interfaces */
static void *csp_router_task(void *param) {
    (void)param;

    printf("CSP router task started\n");

    /* Continuously process routing */
    while (running) {
        csp_route_work();
    }

    return NULL;
}

/* CSP server task - handles incoming connections */
static void *csp_server_task(void *param) {
    node_config_t *node = (node_config_t *)param;

    printf("CSP server task started\n");
    fprintf(log_file, "[SERVER] CSP server task started\n");
    fflush(log_file);

    /* Create socket */
    csp_socket_t sock = {0};

    /* Bind to all ports */
    csp_bind(&sock, CSP_ANY);

    /* Create backlog */
    csp_listen(&sock, 10);

    /* Process incoming connections */
    while (running) {
        /* Accept connection with timeout */
        csp_conn_t *conn = csp_accept(&sock, 1000);
        if (!conn) {
            continue;
        }

        /* Log connection details */
        uint32_t timestamp = csp_get_ms();
        fprintf(log_file, "[SERVER] [%u ms] Connection accepted: src=%u dst=%u sport=%u dport=%u\n",
                timestamp, csp_conn_src(conn), csp_conn_dst(conn),
                csp_conn_sport(conn), csp_conn_dport(conn));
        fflush(log_file);

        /* Read packets on connection */
        csp_packet_t *packet;
        while ((packet = csp_read(conn, 100)) != NULL) {
            /* Log packet reception */
            timestamp = csp_get_ms();
            int dport = csp_conn_dport(conn);

            /* Identify packet type */
            const char *packet_type = "UNKNOWN";
            if (dport == CSP_PING) {
                packet_type = "PING";
            } else if (dport == CSP_CMP) {
                packet_type = "CMP";
            } else if (dport == CSP_PS) {
                packet_type = "PS";
            } else if (dport == CSP_MEMFREE) {
                packet_type = "MEMFREE";
            } else if (dport == CSP_BUF_FREE) {
                packet_type = "BUF_FREE";
            } else if (dport == CSP_UPTIME) {
                packet_type = "UPTIME";
            }

            fprintf(log_file, "[SERVER] [%u ms] Received %s request: src=%u dst=%u dport=%u sport=%u size=%u flags=0x%02X\n",
                    timestamp, packet_type, packet->id.src, packet->id.dst,
                    packet->id.dport, packet->id.sport, packet->length, packet->id.flags);
            fflush(log_file);

            /* Handle packet based on destination port */
            switch (csp_conn_dport(conn)) {
                default:
                    /* Use default CSP service handler for ping, echo, etc. */
                    csp_service_handler(packet);
                    break;
            }
        }

        /* Close connection */
        csp_close(conn);

        timestamp = csp_get_ms();
        fprintf(log_file, "[SERVER] [%u ms] Connection closed\n", timestamp);
        fflush(log_file);
    }

    fprintf(log_file, "[SERVER] CSP server task shutting down\n");
    fflush(log_file);

    return NULL;
}

/* Control interface task - handles commands from orchestrator */
static void *control_interface_task(void *param) {
    topology_config_t *topology = (topology_config_t *)param;

    /* Get control hub configuration */
    int control_port = 5555; // Default port
    const char *control_host = "localhost";

    fprintf(log_file, "[CONTROL] Starting control interface task\n");
    fflush(log_file);

    /* Create ZMQ context and DEALER socket */
    void *zmq_ctx = zmq_ctx_new();
    void *dealer = zmq_socket(zmq_ctx, ZMQ_DEALER);

    /* Set socket identity to node name */
    zmq_setsockopt(dealer, ZMQ_IDENTITY, node_name, strlen(node_name));

    /* Connect to control hub */
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "tcp://%s:%d", control_host, control_port);

    if (zmq_connect(dealer, endpoint) != 0) {
        fprintf(log_file, "[CONTROL] ERROR: Failed to connect to control hub at %s\n", endpoint);
        fflush(log_file);
        zmq_close(dealer);
        zmq_ctx_term(zmq_ctx);
        return NULL;
    }

    fprintf(log_file, "[CONTROL] Connected to control hub at %s\n", endpoint);
    fflush(log_file);

    /* Poll for commands */
    zmq_pollitem_t items[] = {{dealer, 0, ZMQ_POLLIN, 0}};

    while (running) {
        int rc = zmq_poll(items, 1, 1000); // 1 second timeout

        if (rc == -1) {
            break; // Interrupted
        }

        if (items[0].revents & ZMQ_POLLIN) {
            /* Receive command */
            zmq_msg_t msg;
            zmq_msg_init(&msg);

            /* Skip empty delimiter frame */
            zmq_msg_recv(&msg, dealer, 0);
            zmq_msg_close(&msg);

            /* Receive actual message */
            zmq_msg_init(&msg);
            zmq_msg_recv(&msg, dealer, 0);

            char *msg_str = (char *)zmq_msg_data(&msg);
            size_t msg_len = zmq_msg_size(&msg);

            /* Null-terminate for JSON parsing */
            char *json_str = malloc(msg_len + 1);
            memcpy(json_str, msg_str, msg_len);
            json_str[msg_len] = '\0';

            fprintf(log_file, "[CONTROL] Received command: %s\n", json_str);
            fflush(log_file);

            /* Parse JSON command */
            cJSON *cmd = cJSON_Parse(json_str);
            if (cmd) {
                cJSON *command = cJSON_GetObjectItem(cmd, "command");
                cJSON *params = cJSON_GetObjectItem(cmd, "params");

                if (command && cJSON_IsString(command)) {
                    const char *cmd_str = command->valuestring;

                    /* Handle csp_ping command */
                    if (strcmp(cmd_str, "csp_ping") == 0 && params) {
                        cJSON *dest = cJSON_GetObjectItem(params, "destination");
                        cJSON *timeout = cJSON_GetObjectItem(params, "timeout_ms");
                        cJSON *size = cJSON_GetObjectItem(params, "size");
                        cJSON *count = cJSON_GetObjectItem(params, "count");
                        cJSON *trace_param = cJSON_GetObjectItem(params, "trace");

                        if (dest && cJSON_IsNumber(dest)) {
                            int dest_addr = dest->valueint;
                            int timeout_ms = timeout && cJSON_IsNumber(timeout) ? timeout->valueint : 1000;
                            int ping_size = size && cJSON_IsNumber(size) ? size->valueint : 100;
                            int ping_count = count && cJSON_IsNumber(count) ? count->valueint : 1;
                            int trace_enabled = trace_param && cJSON_IsBool(trace_param) && cJSON_IsTrue(trace_param);

                            /* Set connection options - enable trace if requested */
                            uint32_t conn_opts = CSP_O_NONE;
#if CSP_TRACEROUTE
                            if (trace_enabled) {
                                conn_opts |= CSP_O_TRACE;
                            }
#endif

                            fprintf(log_file, "[CONTROL] Executing csp_ping to %d (count=%d, timeout=%d, size=%d, trace=%d, conn_opts=0x%04X)\n",
                                    dest_addr, ping_count, timeout_ms, ping_size, trace_enabled, conn_opts);
                            fflush(log_file);

                            /* Execute ping */
                            int success = 0;
                            int total_time = 0;
                            int min_time = INT_MAX;
                            int max_time = 0;

                            for (int i = 0; i < ping_count; i++) {
                                int result = csp_ping(dest_addr, timeout_ms, ping_size, conn_opts);

                                fprintf(log_file, "[CONTROL] Ping %d/%d to %d: %d ms\n",
                                        i+1, ping_count, dest_addr, result);
                                fflush(log_file);

                                if (result >= 0) {
                                    success++;
                                    total_time += result;
                                    if (result < min_time) min_time = result;
                                    if (result > max_time) max_time = result;
                                }
                            }

                            /* Build response */
                            cJSON *response = cJSON_CreateObject();
                            cJSON_AddStringToObject(response, "source_node", node_name);
                            cJSON_AddStringToObject(response, "command", "csp_ping");
                            cJSON_AddStringToObject(response, "status", success > 0 ? "success" : "failure");

                            cJSON *result_obj = cJSON_CreateObject();
                            cJSON_AddNumberToObject(result_obj, "sent", ping_count);
                            cJSON_AddNumberToObject(result_obj, "received", success);
                            cJSON_AddNumberToObject(result_obj, "avg_time_ms", success > 0 ? total_time / success : 0);
                            cJSON_AddNumberToObject(result_obj, "min_time_ms", success > 0 ? min_time : 0);
                            cJSON_AddNumberToObject(result_obj, "max_time_ms", success > 0 ? max_time : 0);
                            cJSON_AddItemToObject(response, "result", result_obj);

                            char *response_str = cJSON_PrintUnformatted(response);

                            fprintf(log_file, "[CONTROL] Sending response: %s\n", response_str);
                            fflush(log_file);

                            /* Send response back through control hub */
                            zmq_send(dealer, "", 0, ZMQ_SNDMORE);
                            zmq_send(dealer, response_str, strlen(response_str), 0);

                            free(response_str);
                            cJSON_Delete(response);
                        }
                    }
                    /* Handle stats command */
                    else if (strcmp(cmd_str, "stats") == 0) {
                        fprintf(log_file, "[CONTROL] Executing stats command\n");
                        fflush(log_file);

                        /* Log stats to file */
                        log_csp_stats();

                        /* Build response */
                        cJSON *response = cJSON_CreateObject();
                        cJSON_AddStringToObject(response, "source_node", node_name);
                        cJSON_AddStringToObject(response, "command", "stats");
                        cJSON_AddStringToObject(response, "status", "success");
                        cJSON_AddStringToObject(response, "message", "Statistics logged to file");

                        char *response_str = cJSON_PrintUnformatted(response);

                        fprintf(log_file, "[CONTROL] Sending response: %s\n", response_str);
                        fflush(log_file);

                        /* Send response back through control hub */
                        zmq_send(dealer, "", 0, ZMQ_SNDMORE);
                        zmq_send(dealer, response_str, strlen(response_str), 0);

                        free(response_str);
                        cJSON_Delete(response);
                    }
                }

                cJSON_Delete(cmd);
            }

            free(json_str);
            zmq_msg_close(&msg);
        }
    }

    fprintf(log_file, "[CONTROL] Control interface task shutting down\n");
    fflush(log_file);

    zmq_close(dealer);
    zmq_ctx_term(zmq_ctx);

    return NULL;
}
#endif

/* Print usage information */
static void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("\nOptions:\n");
    printf("  -c, --config FILE    Path to topology JSON configuration file (required)\n");
    printf("  -n, --node NAME      Name of this node in the topology (required)\n");
    printf("  -h, --help           Show this help message\n");
    printf("\nExample:\n");
    printf("  %s --config topology.json --node node1\n", prog_name);
}

int main(int argc, char **argv) {
    char *config_file = NULL;
    char *node_name_arg = NULL;

    /* Parse command line arguments */
    static struct option long_options[] = {
        {"config", required_argument, 0, 'c'},
        {"node",   required_argument, 0, 'n'},
        {"help",   no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:n:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                config_file = optarg;
                break;
            case 'n':
                node_name_arg = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* Validate required arguments */
    if (!config_file || !node_name_arg) {
        fprintf(stderr, "Error: Both --config and --node are required\n\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Set global node name */
    node_name = strdup(node_name_arg);

    printf("CSP Virtual Node\n");
    printf("================\n");
    printf("Config file: %s\n", config_file);
    printf("Node name: %s\n", node_name);

    /* Parse topology configuration */
    topology_config_t *topology = parse_topology_config(config_file);
    if (!topology) {
        fprintf(stderr, "Error: Failed to parse topology configuration\n");
        return 1;
    }

    /* Create log directory based on topology name */
    char log_dir[256];
    sanitize_dirname(topology->name, log_dir, sizeof(log_dir));

    /* Create directory if it doesn't exist (mode 0755) */
    struct stat st = {0};
    if (stat(log_dir, &st) == -1) {
        if (mkdir(log_dir, 0755) != 0) {
            fprintf(stderr, "Warning: Failed to create log directory '%s', using current directory\n", log_dir);
            log_dir[0] = '\0';  /* Use current directory */
        }
    }

    /* Open log file for this node */
    char log_filename[512];
    if (log_dir[0] != '\0') {
        snprintf(log_filename, sizeof(log_filename), "%s/%s.log", log_dir, node_name);
    } else {
        snprintf(log_filename, sizeof(log_filename), "%s.log", node_name);
    }

    log_file = fopen(log_filename, "a");
    if (!log_file) {
        fprintf(stderr, "Error: Failed to open log file %s\n", log_filename);
        free_topology_config(topology);
        return 1;
    }

    fprintf(log_file, "\n========================================\n");
    fprintf(log_file, "CSP Virtual Node Starting\n");
    fprintf(log_file, "Node: %s\n", node_name);
    fprintf(log_file, "Topology: %s\n", topology->name);
    fprintf(log_file, "Config: %s\n", config_file);
    fprintf(log_file, "========================================\n");
    fflush(log_file);

    printf("Log file: %s\n\n", log_filename);

    /* Get this node's configuration */
    node_config_t *node = get_node_config(topology, node_name);
    if (!node) {
        fprintf(stderr, "Error: Node '%s' not found in topology\n", node_name);
        fprintf(log_file, "[ERROR] Node '%s' not found in topology\n", node_name);
        fflush(log_file);
        fclose(log_file);
        free_topology_config(topology);
        return 1;
    }

    /* Validate topology */
    if (validate_topology_config(topology) != 0) {
        fprintf(stderr, "Error: Topology validation failed\n");
        free_topology_config(topology);
        return 1;
    }

    printf("Node configuration loaded successfully:\n");
    printf("  Name: %s\n", node->name);
    printf("  Description: %s\n", node->description);
    printf("  Interfaces: %d\n", node->num_interfaces);
    printf("  Routing entries: %d\n", node->num_routes);
    printf("  Services: ping=%s, echo=%s\n",
           node->enable_ping ? "enabled" : "disabled",
           node->enable_echo ? "enabled" : "disabled");

    /* Display interface information */
    printf("\nInterfaces:\n");
    for (int i = 0; i < node->num_interfaces; i++) {
        interface_config_t *iface = &node->interfaces[i];
        printf("  [%d] %s: address=%u, netmask=%u, proxy=%s%s\n",
               i, iface->name, iface->address, iface->netmask,
               iface->zmq_proxy ? iface->zmq_proxy : "default",
               iface->is_default ? " (default)" : "");
    }

    /* Display routing table */
    if (node->num_routes > 0) {
        printf("\nRouting table:\n");
        char *rtable_str = generate_rtable_string(node);
        if (rtable_str) {
            printf("  %s\n", rtable_str);
            free(rtable_str);
        }
    }

#ifdef CSP_ES_AVAILABLE
    printf("\nInitializing CSP...\n");

    /* Set CSP version from topology config */
    csp_conf.version = topology->csp_version;

    /* Set deduplication mode from topology config */
    if (strcmp(topology->deduplication, "all") == 0) {
        csp_conf.dedup = CSP_DEDUP_ALL;
        printf("Deduplication mode: ALL (incoming and forwarding)\n");
    } else if (strcmp(topology->deduplication, "fwd") == 0) {
        csp_conf.dedup = CSP_DEDUP_FWD;
        printf("Deduplication mode: FWD (forwarding only)\n");
    } else if (strcmp(topology->deduplication, "incoming") == 0) {
        csp_conf.dedup = CSP_DEDUP_INCOMING;
        printf("Deduplication mode: INCOMING (incoming only)\n");
    } else {
        csp_conf.dedup = CSP_DEDUP_OFF;
        printf("Deduplication mode: OFF\n");
    }

    /* Initialize CSP */
    csp_init();

#if CSP_TRACEROUTE
    /* Initialize traceroute with ZMQ callback */
    trace_zmq_ctx = zmq_ctx_new();
    if (trace_zmq_ctx) {
        trace_zmq_socket = zmq_socket(trace_zmq_ctx, ZMQ_PUSH);
        if (trace_zmq_socket) {
            /* Connect to trace collector on port 5570 */
            if (zmq_connect(trace_zmq_socket, "tcp://localhost:5570") == 0) {
                csp_traceroute_init(trace_callback_zmq);
                printf("Traceroute enabled, connected to collector on port 5570\n");
                fprintf(log_file, "[INIT] Traceroute enabled, connected to collector on port 5570\n");
            } else {
                fprintf(log_file, "[WARN] Failed to connect trace socket to collector\n");
            }
        }
    }
    fflush(log_file);
#endif

    /* Start CSP router task - CRITICAL for multi-hop routing */
    printf("Starting CSP router task...\n");
    pthread_t router_thread;
    pthread_attr_t router_attr;
    pthread_attr_init(&router_attr);
    pthread_attr_setdetachstate(&router_attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&router_thread, &router_attr, csp_router_task, NULL) != 0) {
        fprintf(stderr, "ERROR: Failed to start CSP router task\n");
        free_topology_config(topology);
        return 1;
    }
    pthread_attr_destroy(&router_attr);

    /* Setup ZMQ interfaces */
    printf("Setting up %d ZMQ interface(s)...\n", node->num_interfaces);
    fprintf(log_file, "[INIT] Setting up %d ZMQ interface(s)...\n", node->num_interfaces);
    fflush(log_file);

    for (int i = 0; i < node->num_interfaces; i++) {
        interface_config_t *iface = &node->interfaces[i];
        csp_iface_t *csp_iface = NULL;

        /* Find the ZMQ proxy configuration */
        zmq_proxy_config_t *proxy = NULL;
        for (int j = 0; j < topology->num_proxies; j++) {
            if (strcmp(topology->zmq_proxies[j].name, iface->zmq_proxy) == 0) {
                proxy = &topology->zmq_proxies[j];
                break;
            }
        }

        if (!proxy) {
            fprintf(stderr, "Error: ZMQ proxy '%s' not found\n", iface->zmq_proxy);
            fprintf(log_file, "[ERROR] ZMQ proxy '%s' not found\n", iface->zmq_proxy);
            fflush(log_file);
            free_topology_config(topology);
            return 1;
        }

        fprintf(log_file, "[INIT] Initializing interface [%d] %s: addr=%u, proxy=%s, ports=%u/%u\n",
                i, iface->name, iface->address, proxy->name, proxy->subscribe_port, proxy->publish_port);
        fflush(log_file);

        /* Initialize ZMQ interface with unique name for proper isolation */
        /* Use csp_zmqhub_init_filter2 to specify interface name and proxy ports */
        int ret = csp_zmqhub_init_filter2(
            iface->name,              /* Interface name (unique per interface) */
            proxy->host,              /* ZMQ proxy host */
            iface->address,           /* CSP address for this interface */
            iface->netmask,           /* Network mask */
            1,                        /* Promiscuous mode (enabled for proper routing) */
            &csp_iface,              /* Return interface pointer */
            NULL,                     /* Security key (not used) */
            proxy->subscribe_port,    /* Proxy subscribe port (for TX) */
            proxy->publish_port       /* Proxy publish port (for RX) */
        );

        if (ret != CSP_ERR_NONE) {
            fprintf(stderr, "Error: Failed to initialize ZMQ interface %s (error %d)\n",
                    iface->name, ret);
            fprintf(log_file, "[ERROR] Failed to initialize ZMQ interface %s (error %d)\n",
                    iface->name, ret);
            fflush(log_file);
            free_topology_config(topology);
            return 1;
        }

        /* Set default flag */
        csp_iface->is_default = iface->is_default ? 1 : 0;

        printf("  [%d] %s: address=%u, host=%s, ports=%u/%u%s\n",
               i, iface->name, iface->address, proxy->host,
               proxy->subscribe_port, proxy->publish_port,
               iface->is_default ? " (default)" : "");

        fprintf(log_file, "[INIT] Interface [%d] %s initialized successfully\n", i, iface->name);
        fprintf(log_file, "[INIT] Interface address: config=%u, csp_iface->addr=%u\n", iface->address, csp_iface->addr);
        fflush(log_file);
    }

    /* Configure routing table */
    if (node->num_routes > 0) {
        printf("\nConfiguring routing table...\n");
        fprintf(log_file, "[INIT] Configuring routing table...\n");
        char *rtable_str = generate_rtable_string(node);
        if (rtable_str) {
            fprintf(log_file, "[INIT] Routing table string: \"%s\"\n", rtable_str);
            fflush(log_file);

            int loaded = csp_rtable_load(rtable_str);
            if (loaded < 1) {
                fprintf(stderr, "Error: Failed to load routing table\n");
                fprintf(log_file, "[ERROR] Failed to load routing table (loaded=%d)\n", loaded);
                fflush(log_file);
                free(rtable_str);
                free_topology_config(topology);
                return 1;
            }
            printf("  Loaded %d routing entries\n", loaded);
            fprintf(log_file, "[INIT] Loaded %d routing entries\n", loaded);
            fflush(log_file);
            free(rtable_str);
        }
    }

    /* Print CSP status */
    printf("\nCSP Status:\n");
    printf("  Version: %d\n", csp_conf.version);
    printf("  Address: %u\n", node->interfaces[0].address);

    printf("\nInterfaces:\n");
    csp_iflist_print();

    printf("\nRouting table:\n");
    csp_rtable_print();

    /* Log interface list to file */
    fprintf(log_file, "[INIT] CSP Interface List:\n");
    csp_iface_t *iface = csp_iflist_get();
    int iface_idx = 0;
    while (iface) {
        fprintf(log_file, "[INIT]   [%d] %s: addr=%u, netmask=%u, default=%s\n",
                iface_idx, iface->name, iface->addr, iface->netmask,
                iface->is_default ? "yes" : "no");
        iface = iface->next;
        iface_idx++;
    }
    fflush(log_file);

    /* Enable promiscuous mode if configured */
    if (topology->monitoring.promiscuous_mode) {
        printf("\nEnabling promiscuous mode (queue size: 100)...\n");
        csp_promisc_enable(100);
    }

    printf("\nCSP initialized successfully!\n");

    /* Start server task */
    pthread_t server_thread;
    if (pthread_create(&server_thread, NULL, csp_server_task, node) != 0) {
        fprintf(stderr, "Error: Failed to create server thread\n");
        fprintf(log_file, "[ERROR] Failed to create server thread\n");
        fflush(log_file);
        fclose(log_file);
        free_topology_config(topology);
        return 1;
    }

    /* Start control interface task */
    pthread_t control_thread;
    if (pthread_create(&control_thread, NULL, control_interface_task, topology) != 0) {
        fprintf(stderr, "Error: Failed to create control interface thread\n");
        fprintf(log_file, "[ERROR] Failed to create control interface thread\n");
        fflush(log_file);
        fclose(log_file);
        free_topology_config(topology);
        return 1;
    }

    fprintf(log_file, "[INFO] All threads started successfully\n");
    fflush(log_file);

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Setup crash handlers for fatal signals */
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGBUS, crash_handler);

    printf("\nNode ready. Press Ctrl+C to exit.\n");

    /* Main loop - handle promiscuous mode packets and periodic stats */
    uint32_t last_stats_time = csp_get_ms();
    const uint32_t stats_interval_ms = 10000; // Log stats every 10 seconds

    while (running) {
        if (topology->monitoring.promiscuous_mode) {
            /* Read promiscuous packets */
            csp_packet_t *packet = csp_promisc_read(1000);
            if (packet) {
                uint32_t timestamp = csp_get_ms();

                /* Log to both stdout and log file */
                printf("[PROMISC] [%u ms] src=%u dst=%u dport=%u sport=%u size=%u\n",
                       timestamp, packet->id.src, packet->id.dst, packet->id.dport,
                       packet->id.sport, packet->length);

                fprintf(log_file, "[PROMISC] [%u ms] src=%u dst=%u dport=%u sport=%u size=%u\n",
                        timestamp, packet->id.src, packet->id.dst, packet->id.dport,
                        packet->id.sport, packet->length);
                fflush(log_file);

                csp_buffer_free(packet);
            }
        } else {
            sleep(1);
        }

        /* Periodic stats logging - DISABLED, use control command instead */
        /* uint32_t current_time = csp_get_ms();
        if (current_time - last_stats_time >= stats_interval_ms) {
            log_csp_stats();
            last_stats_time = current_time;
        } */
    }

    /* Wait for server thread to finish */
    pthread_join(server_thread, NULL);
#else
    printf("\nCSP library not available - running in config-only mode\n");

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Setup crash handlers for fatal signals */
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGBUS, crash_handler);

    printf("\nNode ready. Press Ctrl+C to exit.\n");

    /* Main loop */
    while (running) {
        sleep(1);
    }
#endif

    printf("Shutting down...\n");

    fprintf(log_file, "[INFO] Node shutting down\n");
    fflush(log_file);

    /* Cleanup */
    free_topology_config(topology);

    if (log_file) {
        fprintf(log_file, "[INFO] Shutdown complete\n");
        fclose(log_file);
    }

    if (node_name) {
        free(node_name);
    }

    printf("Goodbye!\n");
    return 0;
}

