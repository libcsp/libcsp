#include "csp_cmp_internal.h"

#include <string.h>

#ifdef __AVR__
static uint32_t wrap_32bit_memcpy(uint32_t to, const uint32_t from, size_t size) {
	return (uint32_t)(uintptr_t)memcpy((void *)(uintptr_t)to, (const void *)(uintptr_t)from, size);
}
static csp_memcpy_fnc_t cmp_memcpy_fnc = wrap_32bit_memcpy;
#else
static csp_memcpy_fnc_t cmp_memcpy_fnc = (csp_memcpy_fnc_t)memcpy;
#endif

static csp_memread64_fnc_t cmp_memread64_fnc = (csp_memread64_fnc_t)NULL;
static csp_memwrite64_fnc_t cmp_memwrite64_fnc = (csp_memwrite64_fnc_t)NULL;

void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc) {
	cmp_memcpy_fnc = fnc;
}

void csp_cmp_set_memread64(csp_memread64_fnc_t fnc) {
	cmp_memread64_fnc = fnc;
}

void csp_cmp_set_memwrite64(csp_memwrite64_fnc_t fnc) {
	cmp_memwrite64_fnc = fnc;
}

csp_memcpy_fnc_t csp_cmp_get_memcpy(void) {
	return cmp_memcpy_fnc;
}

csp_memread64_fnc_t csp_cmp_get_memread64(void) {
	return cmp_memread64_fnc;
}

csp_memwrite64_fnc_t csp_cmp_get_memwrite64(void) {
	return cmp_memwrite64_fnc;
}
