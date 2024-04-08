#include <stdbool.h>
#include <zephyr/kernel.h>

K_MUTEX_DEFINE(atomic_lock);
bool __atomic_compare_exchange_4(volatile void * ptr, void * expected, unsigned int desired,
								 bool weak, int success_memorder, int failure_memorder) {
	bool ret;

	k_mutex_lock(&atomic_lock, K_MSEC(CONFIG_CSP_ATOMIC_MUTEX_TIMEOUT));

	if (*(unsigned int *)ptr == *(unsigned int *)expected) {
		*(unsigned int *)ptr = desired;
		ret = true;
	} else {
		*(unsigned int *)expected = *(unsigned int *)ptr;
		ret = false;
	}

	k_mutex_unlock(&atomic_lock);

	return ret;
}
