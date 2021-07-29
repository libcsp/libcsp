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

#ifndef _CSP_UTIL_H_
#define _CSP_UTIL_H_

#include <csp/csp_util_internal.h>

/* IS_ENABLED(FOO) expands to 1 if defined to 1, or 0 otherwise even
 * if not defined.  This allows us to use defines in C functions
 * rather than #ifdef, which is much cleaner and compiler can verify
 * the code even if an option is not defined. This macro is originated
 * in Linux kernel and used in many open source projects. */
#ifndef IS_ENABLED
#define IS_ENABLED(config) CSP_IS_ENABLED1(config)
#endif

#endif /* _CSP_UTIL_H_ */
