

#include <stdio.h>
#include <inttypes.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/csp_id.h>
#include <csp/csp_iflist.h>
#include <csp/interfaces/csp_if_lo.h>

int csp_rtable_set_internal(uint16_t address, uint16_t netmask, csp_iface_t * ifc, uint16_t via);

static int csp_rtable_parse(const char * rtable, int dry_run) {

	int valid_entries = 0;

	/* Copy string before running strtok */
	const size_t str_len = strnlen(rtable, 100);
	char rtable_copy[str_len + 1];
	strncpy(rtable_copy, rtable, str_len);
	rtable_copy[str_len] = 0;

	/* Get first token */
	char * saveptr;
	char * str = strtok_r(rtable_copy, ",", &saveptr);
	while ((str) && (strlen(str) > 1)) {
		unsigned int address, via;
		int netmask;
		char name[15];
		if (sscanf(str, "%u/%d %14s %u", &address, &netmask, name, &via) == 4) {
		} else if (sscanf(str, "%u/%d %14s", &address, &netmask, name) == 3) {
			via = CSP_NO_VIA_ADDRESS;
		} else if (sscanf(str, "%u %14s %u", &address, name, &via) == 3) {
			netmask = csp_id_get_host_bits();
		} else if (sscanf(str, "%u %14s", &address, name) == 2) {
			netmask = csp_id_get_host_bits();
			via = CSP_NO_VIA_ADDRESS;
		} else {
			// invalid entry
			name[0] = 0;
		}
		name[sizeof(name) - 1] = 0;

		csp_iface_t * ifc = csp_iflist_get_by_name(name);
		if ((address > csp_id_get_max_nodeid()) || (netmask > (int)csp_id_get_host_bits()) || (ifc == NULL)) {
			csp_dbg_errno = CSP_DBG_ERR_INVALID_RTABLE_ENTRY; 
			return CSP_ERR_INVAL;
		}

		if (dry_run == 0) {
			int res = csp_rtable_set(address, netmask, ifc, via);
			if (res != CSP_ERR_NONE) {
				csp_dbg_errno = CSP_DBG_ERR_INVALID_RTABLE_ENTRY;
				return res;
			}
		}
		valid_entries++;
		str = strtok_r(NULL, ",", &saveptr);
	}

	return valid_entries;
}

int csp_rtable_load(const char * rtable) {
	return csp_rtable_parse(rtable, 0);
}

int csp_rtable_check(const char * rtable) {
	return csp_rtable_parse(rtable, 1);
}

int csp_rtable_set(uint16_t address, int netmask, csp_iface_t * ifc, uint16_t via) {

	if ((netmask < 0) || (netmask > (int)csp_id_get_host_bits())) {
		netmask = csp_id_get_host_bits();
	}

	/* Validates options */
	if ((ifc == NULL) || (netmask > (int)csp_id_get_host_bits())) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_RTABLE_ENTRY; 
		return CSP_ERR_INVAL;
	}

	return csp_rtable_set_internal(address, netmask, ifc, via);
}

typedef struct {
	char * buffer;
	size_t len;
	size_t maxlen;
	int error;
} csp_rtable_save_ctx_t;

static bool csp_rtable_save_route(void * vctx, csp_route_t * route) {
	csp_rtable_save_ctx_t * ctx = vctx;

	// Do not save loop back interface
	if (strcmp(route->iface->name, CSP_IF_LOOPBACK_NAME) == 0) {
		return true;
	}

	const char * sep = (ctx->len == 0) ? "" : ",";

	char mask_str[10];
	if (route->netmask != csp_id_get_host_bits()) {
		snprintf(mask_str, sizeof(mask_str), "/%u", route->netmask);
	} else {
		mask_str[0] = 0;
	}
	char via_str[10];
	if (route->via != CSP_NO_VIA_ADDRESS) {
		snprintf(via_str, sizeof(via_str), " %u", route->via);
	} else {
		via_str[0] = 0;
	}
	size_t remain_buf_size = ctx->maxlen - ctx->len;
	int res = snprintf(ctx->buffer + ctx->len, remain_buf_size,
					   "%s%u%s %s%s", sep, route->address, mask_str, route->iface->name, via_str);
	if ((res < 0) || (res >= (int)(remain_buf_size))) {
		ctx->error = CSP_ERR_NOMEM;
		return false;
	}
	ctx->len += res;
	return true;
}

int csp_rtable_save(char * buffer, size_t maxlen) {
	csp_rtable_save_ctx_t ctx = {.len = 0, .buffer = buffer, .maxlen = maxlen, .error = CSP_ERR_NONE};
	buffer[0] = 0;
	csp_rtable_iterate(csp_rtable_save_route, &ctx);
	return ctx.error;
}

void csp_rtable_clear(void) {
	csp_rtable_free();
}

#if (CSP_DEBUG)

static bool csp_rtable_print_route(void * ctx, csp_route_t * route) {
	if (route->via == CSP_NO_VIA_ADDRESS) {
		printf("%u/%u %s\r\n", route->address, route->netmask, route->iface->name);
	} else {
		printf("%u/%u %s %u\r\n", route->address, route->netmask, route->iface->name, route->via);
	}
	return true;
}

void csp_rtable_print(void) {
	csp_rtable_iterate(csp_rtable_print_route, NULL);
}

#endif
