#include "csp_cmp_internal.h"

#include "csp_macro.h"

#include <csp/csp_hooks.h>

#include <string.h>

#ifdef __AVR__
__weak int csp_cmp_memcpy(csp_memptr_t to, csp_const_memptr_t from, size_t size) {
	memcpy((void *)(uintptr_t)to, (const void *)(uintptr_t)from, size);
	return CSP_ERR_NONE;
}
#else
__weak int csp_cmp_memcpy(csp_memptr_t to, csp_const_memptr_t from, size_t size) {
	memcpy(to, from, size);
	return CSP_ERR_NONE;
}
#endif

__weak int csp_cmp_memread64(csp_const_memptr_t to, csp_memptr64_t from, size_t size) {
	(void)to;
	(void)from;
	(void)size;
	return CSP_ERR_DRIVER;
}

__weak int csp_cmp_memwrite64(csp_memptr64_t to, csp_memptr_t from, size_t size) {
	(void)to;
	(void)from;
	(void)size;
	return CSP_ERR_DRIVER;
}

void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc) {
	(void)fnc;
}

void csp_cmp_set_memread64(csp_memread64_fnc_t fnc) {
	(void)fnc;
}

void csp_cmp_set_memwrite64(csp_memwrite64_fnc_t fnc) {
	(void)fnc;
}
