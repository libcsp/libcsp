#!/usr/bin/env python
# encoding: utf-8

# Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
# Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
# Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk) 
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

import os

APPNAME = 'libcsp'
VERSION = '1.0'

top	= '.'
out	= 'build'

def options(ctx):
	# Load GCC options
	ctx.load('gcc')
	
	ctx.add_option('--toolchain', default='', help='Set toolchain prefix')

	# Set libcsp options
	gr = ctx.add_option_group('libcsp options')
	gr.add_option('--cflags', default='', help='Add additional CFLAGS. Separate with comma')
	gr.add_option('--includes', default='', help='Add additional include paths. Separate with comma')
	
	gr.add_option('--disable-output', action='store_true', help='Disable CSP output')
	gr.add_option('--enable-rdp', action='store_true', help='Enable RDP support')
	gr.add_option('--enable-qos', action='store_true', help='Enable Quality of Service support')
	gr.add_option('--enable-promisc', action='store_true', help='Enable promiscuous mode support')
	gr.add_option('--enable-crc32', action='store_true', help='Enable CRC32 support')
	gr.add_option('--enable-hmac', action='store_true', help='Enable HMAC-SHA1 support')
	gr.add_option('--enable-xtea', action='store_true', help='Enable XTEA support')
	gr.add_option('--enable-bindings', action='store_true', help='Enable Python bindings')
	gr.add_option('--enable-examples', action='store_true', help='Enable examples')
	gr.add_option('--enable-static-buffer', action='store_true', help='Enable static buffer system')

	gr.add_option('--with-os', default='posix', help='Set operating system. Must be either \'posix\', \'windows\' or \'freertos\'')
	gr.add_option('--with-can', default=None, metavar='CHIP', help='Enable CAN driver. CHIP must be either socketcan, at91sam7a1, at91sam7a3 or at90can128')
	gr.add_option('--with-freertos', metavar="PATH", default='../../libgomspace/include', help='Set path to FreeRTOS header files')
	gr.add_option('--with-static-buffer-size', type=int, default=320, help='Set size of static buffer elements')
	gr.add_option('--with-static-buffer-count', type=int, default=12, help='Set number of static buffer elements')
	gr.add_option('--with-rdp-max-window', type=int, default=20, help='Set maximum window size for RDP')
	gr.add_option('--with-max-bind-port', type=int, default=31, help='Set maximum bindable port')
	gr.add_option('--with-max-connections', type=int, default=10, help='Set maximum number of concurrent connections')
	gr.add_option('--with-conn-queue-length', type=int, default=100, help='Set maximum number of packets in queue for a connection')
	gr.add_option('--with-router-queue-length', type=int, default=10, help='Set maximum number of packets to be queued at the input of the router')
	gr.add_option('--with-padding', type=int, default=8, help='Set padding bytes before packet length field')

def configure(ctx):
	# Validate OS
	if not ctx.options.with_os in ('posix', 'windows', 'freertos'):
		ctx.fatal('--with-os must be either \'posix\', \'windows\' or \'freertos\'')

	# Validate CAN drivers
	if not ctx.options.with_can in (None, 'socketcan', 'at91sam7a1', 'at91sam7a3', 'at90can128'):
		ctx.fatal('--with-can must be either \'socketcan\', \'at91sam7a1\', \'at91sam7a3\', \'at90can128\'')

	# Setup and validate toolchain
	ctx.env.CC = ctx.options.toolchain + 'gcc'
	ctx.env.AR = ctx.options.toolchain + 'ar'
	ctx.env.SIZE = ctx.options.toolchain + 'size'
	ctx.load('gcc')
	ctx.find_program('size', var='SIZE')

	# Set git revision define
	git_rev = os.popen("(git log --pretty=format:%H -n 1 | egrep -o \"^.{8}\") 2> /dev/null || echo none").read().strip()

	# Setup DEFINES
	ctx.env.append_unique('DEFINES_CSP', ['GIT_REV="{0}"'.format(git_rev)])

	# Setup CFLAGS
	ctx.env.append_unique('CFLAGS_CSP', ['-Os','-Wall', '-g', '-std=gnu99'] + ctx.options.cflags.split(','))
	
	# Setup extra includes
	ctx.env.append_unique('INCLUDES_CSP', ['include'] + ctx.options.includes.split(','))

	# Add default files
	ctx.env.append_unique('FILES_CSP', ['src/*.c','src/interfaces/csp_if_lo.c','src/transport/csp_udp.c','src/arch/{0}/**/*.c'.format(ctx.options.with_os)])

	# Add FreeRTOS 
	if ctx.options.with_os == 'freertos':
		ctx.env.append_unique('INCLUDES_CSP', ctx.options.with_freertos)
		ctx.define('_CSP_FREERTOS_', 1)
	elif ctx.options.with_os == 'posix':
		ctx.define('_CSP_POSIX_', 1)
	elif ctx.options.with_os == 'windows':
		ctx.define('_CSP_WINDOWS_', 1)
		ctx.env.append_unique('CFLAGS_CSP', ['-D_WIN32_WINNT=0x0600'] + ctx.options.cflags.split(','))
	else:
		ctx.fatal('ARCH must be either \'posix\', \'windows\' or \'freertos\'')

	# Add CAN driver
	if ctx.options.with_can:
		ctx.env.append_unique('FILES_CSP', 'src/interfaces/csp_if_can.c')
		ctx.env.append_unique('FILES_CSP', 'src/drivers/can/can_{0}.c'.format(ctx.options.with_can))

	# Store configuration options
	ctx.env.ENABLE_BINDINGS = ctx.options.enable_bindings
	ctx.env.ENABLE_EXAMPLES = ctx.options.enable_examples

	# Create config file
	ctx.define_cond('CSP_DEBUG', not ctx.options.disable_output)
	if not ctx.options.disable_output:
		ctx.env.append_unique('FILES_CSP', 'src/csp_debug.c')
	else:
		ctx.env.append_unique('EXCL_CSP', 'src/csp_debug.c')

	ctx.define_cond('CSP_USE_RDP', ctx.options.enable_rdp)
	if ctx.options.enable_rdp:
		ctx.env.append_unique('FILES_CSP', 'src/transport/csp_rdp.c')

	ctx.define_cond('CSP_ENABLE_CRC32', ctx.options.enable_crc32)
	if ctx.options.enable_crc32:
		ctx.env.append_unique('FILES_CSP', 'src/csp_crc32.c')
	else:
		ctx.env.append_unique('EXCL_CSP', 'src/csp_crc32.c')

	if ctx.options.enable_hmac:
		ctx.env.append_unique('FILES_CSP', 'src/crypto/csp_hmac.c')
		ctx.env.append_unique('FILES_CSP', 'src/crypto/csp_sha1.c')
		ctx.define_cond('CSP_ENABLE_HMAC', ctx.options.enable_hmac)

	if ctx.options.enable_xtea:
		ctx.env.append_unique('FILES_CSP', 'src/crypto/csp_xtea.c')
		ctx.env.append_unique('FILES_CSP', 'src/crypto/csp_sha1.c')
		ctx.define_cond('CSP_ENABLE_XTEA', ctx.options.enable_xtea)

	ctx.define_cond('CSP_USE_PROMISC', ctx.options.enable_promisc)
	ctx.define_cond('CSP_USE_QOS', ctx.options.enable_qos)
	ctx.define_cond('CSP_BUFFER_STATIC', ctx.options.enable_static_buffer)
	ctx.define('CSP_BUFFER_COUNT', ctx.options.with_static_buffer_count)
	ctx.define('CSP_BUFFER_SIZE', ctx.options.with_static_buffer_size)
	ctx.define('CSP_CONN_MAX', ctx.options.with_max_connections)
	ctx.define('CSP_CONN_QUEUE_LENGTH', ctx.options.with_conn_queue_length)
	ctx.define('CSP_FIFO_INPUT', ctx.options.with_router_queue_length)
	ctx.define('CSP_MAX_BIND_PORT', ctx.options.with_max_bind_port)
	ctx.define('CSP_RDP_MAX_WINDOW', ctx.options.with_rdp_max_window)
	ctx.define('CSP_PADDING_BYTES', ctx.options.with_padding)

	ctx.write_config_header('include/csp/csp_autoconfig.h', top=False, remove=False)
	
	# Check for endian.h
	ctx.check(header_name='endian.h', mandatory=False)

def build(ctx):
	# Build static library
	ctx.stlib(
		source=ctx.path.ant_glob(ctx.env.FILES_CSP, excl=ctx.env.EXCL_CSP),
		target = 'csp',
		includes= ctx.env.INCLUDES_CSP,
		export_includes = 'include',
		cflags = ctx.env.CFLAGS_CSP,
		defines = ctx.env.DEFINES_CSP,
		use = 'csp_size')

	# Print library size
	if ctx.options.verbose > 0:
		ctx(rule='${SIZE} --format=berkeley ${SRC}', source='libcsp.a', name='csp_size', always=True)

	# Build shared library for Python bindings
	if ctx.env.ENABLE_BINDINGS:
		ctx.shlib(source=ctx.path.ant_glob(ctx.env.FILES_CSP),
			target = 'pycsp',
			includes= ctx.env.INCLUDES_CSP,
			export_includes = 'include',
			cflags = ctx.env.CFLAGS_CSP,
			defines = ctx.env.DEFINES_CSP,
			lib=['rt', 'pthread'])

	if ctx.env.ENABLE_EXAMPLES:
		ctx.program(source = ctx.path.ant_glob('examples/simple.c'),
			target = 'simple',
			includes = ctx.env.INCLUDES_CSP,
			cflags = ctx.env.CFLAGS_CSP,
			defines = ctx.env.DEFINES_CSP,
			lib=['rt', 'pthread'],
			use = 'csp')

	# Set install path for header files
	ctx.install_files('${PREFIX}', ctx.path.ant_glob('include/**/*.h'), relative_trick=True)
	ctx.install_files('${PREFIX}/include/csp', 'include/csp/csp_autoconfig.h')
	ctx.install_files('${PREFIX}/lib', 'libcsp.a')

def dist(ctx):
	ctx.excl = 'build/* **/.* **/*.pyc **/*.o **/*~ *.tar.gz'
