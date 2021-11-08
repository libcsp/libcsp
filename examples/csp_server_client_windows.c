#include <csp/csp.h>
#include <windows.h>
#include <process.h>

void server(void);
void client(void);

static int csp_win_thread_create(unsigned int (* routine)(void *)) {

	uintptr_t ret = _beginthreadex(NULL, 0, routine, NULL, 0, NULL);
	if (ret == 0) {
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;
}

static unsigned int task_router(void * param) {

	/* Here there be routing */
	while (1) {
		csp_route_work();
	}

	return 0;
}

static unsigned int task_server(void * param) {
	server();
	return 0;
}

static unsigned int task_client(void * param) {
	client();
	return 0;
}

int router_start(void) {
	return csp_win_thread_create(task_router);
}

int server_start(void) {
	return csp_win_thread_create(task_server);
}

int client_start(void) {
	return csp_win_thread_create(task_client);
}
