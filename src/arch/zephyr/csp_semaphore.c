#include "../../csp_semaphore.h"
#include <csp/csp_debug.h>

#include <zephyr/kernel.h>

static int csp_bin_sem_errno_to_csp(int val) {
	int ret;

	switch (val) {
		case 0:
			ret = CSP_SEMAPHORE_OK;
			break;
		default:
			ret = CSP_SEMAPHORE_ERROR;
			break;
	}

	return ret;
}

void csp_bin_sem_init(csp_bin_sem_t * sem) {
	struct k_sem * s = (struct k_sem *)sem;

	(void)k_sem_init(s, 1, 1);
}

int csp_bin_sem_wait(csp_bin_sem_t * sem, uint32_t timeout) {
	int ret;
	struct k_sem * s = (struct k_sem *)sem;

	ret = k_sem_take(s, K_MSEC(timeout));

	return csp_bin_sem_errno_to_csp(ret);
}

int csp_bin_sem_post(csp_bin_sem_t * sem) {
	struct k_sem * s = (struct k_sem *)sem;

	k_sem_give(s);

	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_post_isr(csp_bin_sem_t * sem, int * unused) {
	ARG_UNUSED(unused);

	return csp_bin_sem_post(sem);
}
