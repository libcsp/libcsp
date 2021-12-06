#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <pthread.h>

void server(void);
void client(void);

static int csp_pthread_create(void * (*routine)(void *)) {

	pthread_attr_t attributes;
	pthread_t handle;
	int ret;

	if (pthread_attr_init(&attributes) != 0) {
		return CSP_ERR_NOMEM;
	}
	/* no need to join with thread to free its resources */
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&handle, &attributes, routine, NULL);
	pthread_attr_destroy(&attributes);

	if (ret != 0) {
		return ret;
	}

	return CSP_ERR_NONE;
}

static void * task_router(void * param) {

	/* Here there be routing */
	while (1) {
		csp_route_work();
	}

	return NULL;
}

static void * task_server(void * param) {
	server();
	return NULL;
}

static void * task_client(void * param) {
	client();
	return NULL;
}

int router_start(void) {
	return csp_pthread_create(task_router);
}

int server_start(void) {
	return csp_pthread_create(task_server);
}

int client_start(void) {
	return csp_pthread_create(task_client);
}
