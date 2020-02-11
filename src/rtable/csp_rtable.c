/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "csp_rtable_internal.h"

#include <stdio.h>

#include <csp/csp.h>
#include <csp/csp_iflist.h>
#include <csp/interfaces/csp_if_lo.h>

#include "../csp_init.h"

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
		unsigned int address, netmask, via;
		char name[15];
		if (sscanf(str, "%u/%u %14s %u", &address, &netmask, name, &via) == 4) {
		} else if (sscanf(str, "%u/%u %14s", &address, &netmask, name) == 3) {
			via = CSP_NO_VIA_ADDRESS;
		} else if (sscanf(str, "%u %14s %u", &address, name, &via) == 3) {
			netmask = CSP_ID_HOST_SIZE;
		} else if (sscanf(str, "%u %14s", &address, name) == 2) {
			netmask = CSP_ID_HOST_SIZE;
			via = CSP_NO_VIA_ADDRESS;
		} else {
			// invalid entry
			name[0] = 0;
		}
		name[sizeof(name) - 1] = 0;

		csp_iface_t * ifc = csp_iflist_get_by_name(name);
		if ((address > CSP_ID_HOST_MAX) || (netmask > CSP_ID_HOST_SIZE) || (via > UINT8_MAX) || (ifc == NULL))  {
			csp_log_error("%s: invalid entry [%s]", __FUNCTION__, str);
			return CSP_ERR_INVAL;
		}

		if (dry_run == 0) {
			int res = csp_rtable_set(address, netmask, ifc, via);
			if (res != CSP_ERR_NONE) {
				csp_log_error("%s: failed to add [%s], error: %d", __FUNCTION__, str, res);
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

int csp_rtable_set(uint8_t address, uint8_t netmask, csp_iface_t *ifc, uint8_t via) {

	/* Legacy reference to default route (the old way) */
	if (address == CSP_DEFAULT_ROUTE) {
		netmask = 0;
		address = 0;
	}

	/* Validates options */
	if (((address > CSP_ID_HOST_MAX) && (address != 255)) || (ifc == NULL) || (netmask > CSP_ID_HOST_SIZE)) {
		csp_log_error("%s: invalid route: address %u, netmask %u, interface %p (%s), via %u",
                              __FUNCTION__, address, netmask, ifc, (ifc != NULL) ? ifc->name : "", via);
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

static bool csp_rtable_save_route(void * vctx, uint8_t address, uint8_t mask, const csp_route_t * route)
{
    csp_rtable_save_ctx_t * ctx = vctx;

    // Do not save loop back interface
    if (strcasecmp(route->iface->name, CSP_IF_LOOPBACK_NAME) == 0) {
        return true;
    }

    const char * sep = (ctx->len == 0) ? "" : ",";

    char mask_str[10];
    if (mask != CSP_ID_HOST_SIZE) {
        snprintf(mask_str, sizeof(mask_str), "/%u", mask);
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
                       "%s%u%s %s%s", sep, address, mask_str, route->iface->name, via_str);
    if ((res < 0) || (res >= (int)(remain_buf_size))) {
        ctx->error = CSP_ERR_NOMEM;
        return false;
    }
    ctx->len += res;
    return true;
}

int csp_rtable_save(char * buffer, size_t maxlen)
{
    csp_rtable_save_ctx_t ctx = {.len = 0, .buffer = buffer, .maxlen = maxlen, .error = CSP_ERR_NONE};
    buffer[0] = 0;
    csp_rtable_iterate(csp_rtable_save_route, &ctx);
    return ctx.error;
}

void csp_rtable_clear(void) {
	csp_rtable_free();

	/* Set loopback up again */
	csp_rtable_set(csp_conf.address, CSP_ID_HOST_SIZE, &csp_if_lo, CSP_NO_VIA_ADDRESS);
}

#if (CSP_DEBUG)

static bool csp_rtable_print_route(void * ctx, uint8_t address, uint8_t mask, const csp_route_t * route)
{
    if (route->via == CSP_NO_VIA_ADDRESS) {
        printf("%u/%u %s\r\n", address, mask, route->iface->name);
    } else {
        printf("%u/%u %s %u\r\n", address, mask, route->iface->name, route->via);
    }
    return true;
}

void csp_rtable_print(void)
{
    csp_rtable_iterate(csp_rtable_print_route, NULL);
}

#endif
