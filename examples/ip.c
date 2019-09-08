/**
 * Example usage of the IP interface (on loopback)
 */

#include <stdio.h>
#include <stdlib.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_ip.h>
#include <csp/arch/csp_thread.h>

#define SERVER_NODE 2
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 10
#define SERVER_IP_PORT 3001

#define CLIENT_NODE 1
#define CLIENT_IP_PORT 3000

int target_node;


CSP_DEFINE_TASK(server_task) {
    csp_socket_t* socket = csp_socket(CSP_SO_NONE);
    csp_conn_t* conn;
    csp_packet_t* request;
    
    csp_bind(socket, CSP_ANY);
    csp_listen(socket, 5);
    
    printf("Server task started\n");
    
    while (1) {
        // Wait to accept a connection
        if ((conn = csp_accept(socket, 10000)) == NULL) {
            continue;
        }
        
        // Read packets from connection
        while ((request = csp_read(conn, 100)) != NULL) {
            int dport = csp_conn_dport(conn);
            
            if (dport != SERVER_PORT) {
                csp_service_handler(conn, request);
                continue;
            }
            
            printf("SERVER: received: %s\n", (char*)request->data);
            csp_buffer_free(request);
        }
        
        csp_close(conn);
    }
    
    return CSP_TASK_RETURN;
}


CSP_DEFINE_TASK(client_task) {
    
    csp_conn_t* conn;
    csp_packet_t* request;
    
    int i = 0;
    while (1) {
        i++;
        
        conn = csp_connect(CSP_PRIO_NORM, target_node, SERVER_PORT, 1000, CSP_O_NONE);
        if (conn == NULL) { break; }
        
        request = csp_buffer_get(50);
        if (request == NULL) { break; }
        
        char* msg = malloc(50);
        sprintf(msg, "Request %d", i);
        
        strcpy((char*)request->data, msg);
        request->length = strlen(msg);
        
        printf("CLIENT: sending message %s\n", msg);
        if (!csp_send(conn, request, 1000)) {
            printf("Send failed\n");
            csp_buffer_free(request);
        }
        
        free(msg);
        
        csp_sleep_ms(3000);
    }
}



int main(int argc, char** argv) {
    
    printf("Due to routing, you must start this program twice, first \
            as the client (node 1) and second as the server (node 2).\n");
    
    if (argc != 2) {
        printf("Format: ./ip {1 | 2} // 1 for client, 2 for server");
        return 1;
    }
    
    int node = atoi(argv[1]);
    target_node = node == 1 ? 2 : 1;
    
    // Initialize CSP 
    csp_debug_toggle_level(CSP_PACKET);
    
    csp_buffer_init(10, 50);
    csp_init(node);
    
    // Setup IP interface
    csp_ip_config_t ip_config;
    ip_config.tx_addr = SERVER_ADDR;
    ip_config.tx_ip_port = node == 1 ? SERVER_IP_PORT : CLIENT_IP_PORT;
    ip_config.rx_ip_port = node == 1 ? CLIENT_IP_PORT : SERVER_IP_PORT;
    
    csp_ip_init(ip_config);

    // Setup CSP routing   
    csp_route_set(target_node, &csp_if_ip, CSP_NODE_MAC);
    csp_route_start_task(500, 1);
    
    if (node == 1) { 
        csp_thread_handle_t handle_client;
        csp_thread_create(client_task, "CLIENT", 1000, NULL, 0, &handle_client);
    } else {
        csp_thread_handle_t handle_server;
        csp_thread_create(server_task, "SERVER", 1000, NULL, 0, &handle_server);
    }

    /* Wait for program to terminate (ctrl + c) */
    while(1) {
    	csp_sleep_ms(1000000);
    }
    
    return 0;
}