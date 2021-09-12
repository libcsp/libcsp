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

#include "csp_init.h"

#include <csp/interfaces/csp_if_lo.h>
#include <csp/arch/csp_time.h>
#include <csp/csp_id.h>
#include "csp_conn.h"
#include "csp_qfifo.h"
#include "csp_port.h"

csp_conf_t csp_conf;

uint16_t csp_get_address(void) {

	return csp_conf.address;
}

int csp_init(const csp_conf_t * conf) {

	/* make offset first time, so uptime is counted from process/task boot */
	csp_get_uptime_s();

	/* Make a copy of the configuration
	 * The copy is kept hidden for the user in csp_init.h
	 * Configuration cannot be changed after calling init
	 * unless specific get/set functions are made */
	memcpy(&csp_conf, conf, sizeof(csp_conf));

	csp_buffer_init();
	csp_conn_init();
	csp_qfifo_init();

	/* Loopback */
	csp_iflist_add(&csp_if_lo);

	/* Register loopback route */
	csp_rtable_set(csp_conf.address, csp_id_get_host_bits(), &csp_if_lo, CSP_NO_VIA_ADDRESS);

	/* Also register loopback as default, until user redefines default route */
	csp_rtable_set(0, 0, &csp_if_lo, CSP_NO_VIA_ADDRESS);

	return CSP_ERR_NONE;

}

void csp_free_resources(void) {

	csp_rtable_free();

}

const csp_conf_t * csp_get_conf(void) {

	return &csp_conf;

}
