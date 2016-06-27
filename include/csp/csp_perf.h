/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2016 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2016 AAUSAT3 Project (http://aausat3.space.aau.dk)

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

#ifndef _CSP_PERF_H_
#define _CSP_PERF_H_

#include <stdint.h>

#include <csp/csp.h>

#ifdef __cplusplus
extern "C" {
#endif

struct csp_perf_config {
	uint8_t port;
	uint8_t server;
	uint8_t flags;

	unsigned int timeout_ms;
	unsigned int data_size;
	unsigned int runtime;
	unsigned int max_frames;
	unsigned int bandwidth;
	unsigned int update_interval;

	bool should_stop;
};

static inline void csp_perf_set_defaults(struct csp_perf_config *conf)
{
	conf->timeout_ms = 1000;
	conf->data_size = 100;
	conf->runtime = 10;
	conf->max_frames = 0;
	conf->bandwidth = 1000000; /* bits/s */
	conf->update_interval = 1;
	conf->port = 11;
}

static inline void csp_perf_stop(struct csp_perf_config *conf)
{
	conf->should_stop = true;
}

int csp_perf_server(struct csp_perf_config *conf);

int csp_perf_client(struct csp_perf_config *conf);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CSP_PERF_H_ */
