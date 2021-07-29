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

#ifndef _CSP_UTIL_INTERNAL_H_
#define _CSP_UTIL_INTERNAL_H_

/* Convert the define to a numeric or keep the string as-is if not
 * defined This is defined in csp_util.h.  Having it here just for
 * documentation purpose.
 * defined:   1
 * undefined: XXXX
 * #define CSP_IS_ENABLED(config) CSP_IS_ENABLED1(config) */

/*
 * defined:   "__CSP_CONFIG_PREFIX_1" -> '0,'
 * undefined: "__CSP_CONFIG_PREFIX_XXXX"
 */
#define __CSP_CONFIG_PREFIX_1 0,
#define CSP_IS_ENABLED1(config) CSP_IS_ENABLED2(__CSP_CONFIG_PREFIX_##config)

/*
 * defined:   "0, 1, 0"
 * undefined: "__CS_CONFIG_PREFIX_XXXX 1, 0"
 */
#define CSP_IS_ENABLED2(config) CSP_IS_ENABLED3(config 1, 0)

/* Take the second arg; that is 1 if defined or 0 otherwise */
#define CSP_IS_ENABLED3(__ignore, val, ...) val

#endif /* _CSP_UTIL_INTERNAL_H_ */
