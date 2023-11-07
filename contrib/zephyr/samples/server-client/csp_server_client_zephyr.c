#include <zephyr/kernel.h>
#include <csp/csp.h>

void server(void);
void client(void);

#define ROUTER_STACK_SIZE 256
#define SERVER_STACK_SIZE 1024
#define CLIENT_STACK_SIZE 1024
#define ROUTER_PRIO 0
#define SERVER_PRIO 0
#define CLIENT_PRIO 0

static void * router_task(void * param) {

	/* Here there be routing */
	while (1) {
		csp_route_work();
	}

	return NULL;
}

static void * server_task(void * p1, void * p2, void * p3) {

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	server();

	return NULL;
}

static void * client_task(void * p1, void * p2, void * p3) {

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	client();

	return NULL;
}

K_THREAD_DEFINE(router_id, ROUTER_STACK_SIZE,
				router_task, NULL, NULL, NULL,
				ROUTER_PRIO, 0, K_TICKS_FOREVER);
K_THREAD_DEFINE(server_id, SERVER_STACK_SIZE,
				server_task, NULL, NULL, NULL,
				SERVER_PRIO, 0, K_TICKS_FOREVER);
K_THREAD_DEFINE(client_id, CLIENT_STACK_SIZE,
				client_task, NULL, NULL, NULL,
				CLIENT_PRIO, 0, K_TICKS_FOREVER);

void router_start(void) {
	k_thread_start(router_id);
}

void server_start(void) {
	k_thread_start(server_id);
}

void client_start(void) {
	k_thread_start(client_id);
}
