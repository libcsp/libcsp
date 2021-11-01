

#include <csp/arch/csp_thread.h>

csp_thread_handle_t
csp_thread_create_static(csp_thread_handle_t * new_thread,
						 const char * const thread_name,
						 char * stack,
						 unsigned int stack_size,
						 csp_thread_func_t entry,
						 void * param,
						 unsigned int priority) {
	k_tid_t tid;

	tid = k_thread_create(new_thread,
						  stack, stack_size,
						  entry, param, NULL, NULL,
						  priority, 0, K_NO_WAIT);
	k_thread_name_set(tid, thread_name);

	return tid;
}

void csp_thread_exit(void) {
	k_thread_abort(k_current_get());
}

void csp_sleep_ms(unsigned int time_ms) {
	k_sleep(K_MSEC(time_ms));
}
