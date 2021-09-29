/*
 * Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
 * Copyright (C) 2021 Space Cubics, LLC.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; If not, see <http://www.gnu.org/licenses/>.
 */

#include <csp/arch/csp_queue.h>

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(libcsp);

csp_queue_handle_t csp_queue_create_static(int length, size_t item_size,
					   char *buf, csp_static_queue_t *queue)
{
	struct k_msgq *q = (struct k_msgq *)queue;

	k_msgq_init(q, buf, item_size, length);

	return q;
}

static int csp_errno_zephyr_to_csp(int err)
{
	int ret;

	if (err == 0) {
		ret = CSP_QUEUE_OK;
	} else {
		ret = CSP_QUEUE_ERROR;
	}

	return ret;
}

int csp_queue_enqueue(csp_queue_handle_t queue, const void * value, uint32_t timeout)
{
	int ret;
	struct k_msgq *q = (struct k_msgq *)queue;

	ret = k_msgq_put(q, value, K_MSEC(timeout));

	return csp_errno_zephyr_to_csp(ret);
}

int csp_queue_enqueue_isr(csp_queue_handle_t queue, const void *value, int *unused)
{
	int ret;
	struct k_msgq *q = (struct k_msgq *)queue;

	ARG_UNUSED(unused);

	ret = k_msgq_put(q, value, K_NO_WAIT);

	return csp_errno_zephyr_to_csp(ret);
}

int csp_queue_dequeue(csp_queue_handle_t queue, void * buf, uint32_t timeout)
{
	int ret;
	struct k_msgq *q = (struct k_msgq *)queue;

	ret = k_msgq_get(q, buf, K_MSEC(timeout));

	return csp_errno_zephyr_to_csp(ret);
}

int csp_queue_dequeue_isr(csp_queue_handle_t queue, void *buf, int *unused)
{
	int ret;
	struct k_msgq *q = (struct k_msgq *)queue;

	ARG_UNUSED(unused);

	ret = k_msgq_get(q, buf, K_NO_WAIT);

	return csp_errno_zephyr_to_csp(ret);
}

int csp_queue_size(csp_queue_handle_t queue)
{
	struct k_msgq *q = (struct k_msgq *)queue;

	return k_msgq_num_used_get(q);
}

int csp_queue_size_isr(csp_queue_handle_t queue)
{
	return csp_queue_size(queue);
}
