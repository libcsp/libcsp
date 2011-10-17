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

APPNAME = 'libcsp'
VERSION = '1.0-rc1'

def options(ctx):
	# Load GCC options
	ctx.load('gcc')
	ctx.load('eclipse')

	# Set CSP options
	gr = ctx.add_option_group('CSP options')
	gr.add_option('--toolchain', default='', help='Set toolchain prefix')
	gr.add_option('--os', default='posix', help='Set operating system. Must be either \'posix\' or \'freertos\'')
	gr.add_option('--cflags', default='', help='Add additional CFLAGS. Separate with comma')
	gr.add_option('--includes', default='', help='Add additional include paths. Separate with comma')
	gr.add_option('--libdir', default='build', help='Set output directory of libcsp.a')
	gr.add_option('--with-can', default=None, metavar='CHIP', help='Enable CAN driver. CHIP must be either socketcan, at91sam7a1, at91sam7a3 or at90can128')
	gr.add_option('--with-freertos', metavar="PATH", default='../../libgomspace/include', help='Set path to FreeRTOS header files')
	gr.add_option('--with-csp-config', metavar="CONFIG", default=None, help='Set CSP configuration file')
	gr.add_option('--enable-bindings', action='store_true', help='Enable Python bindings')

def configure(ctx):
	# Validate OS
	if not ctx.options.os in ('posix', 'freertos'):
		ctx.fatal('ARCH must be either \'posix\' or \'freertos\'')

	# Validate CAN drivers
	if not ctx.options.with_can in (None, 'socketcan', 'at91sam7a1', 'at91sam7a3', 'at90can128'):
		ctx.fatal('CAN must be either \'socketcan\', \'at91sam7a1\', \'at91sam7a3\', \'at90can128\'')

	# Setup and validate toolchain
	ctx.env.CC = ctx.options.toolchain + 'gcc'
	ctx.env.AR = ctx.options.toolchain + 'ar'
	ctx.load('gcc')
        ctx.find_program(ctx.options.toolchain + 'size', var='SIZE')

	# Setup CFLAGS
	ctx.env.append_unique('CFLAGS_CSP', ['-Os','-Wall', '-g', '-std=gnu99'] + ctx.options.cflags.split(','))
	
	# Setup extra includes
	ctx.env.append_unique('INCLUDES_CSP', ['include'] + ctx.options.includes.split(','))

	# Add default files
	ctx.env.append_unique('FILES_CSP', ['src/*.c','src/crypto/*.c','src/interfaces/csp_if_lo.c','src/transport/*.c','src/arch/{0}/**/*.c'.format(ctx.options.os)])

	# Add FreeRTOS 
	if ctx.options.os == 'freertos':
		ctx.env.append_unique('INCLUDES_CSP', ctx.options.with_freertos)

	# Add CAN driver
	if ctx.options.with_can:
		ctx.env.append_unique('FILES_CSP', 'src/interfaces/csp_if_can.c')
		ctx.env.append_unique('FILES_CSP', 'src/interfaces/can/can_{0}.c'.format(ctx.options.with_can))

	if ctx.options.with_csp_config:
		conf = ctx.path.find_resource(ctx.options.with_csp_config)
		if not conf: ctx.fatal("No such config file: {0}".format(ctx.options.with_csp_config))
		ctx.define('CSP_CONFIG', conf.abspath())

def build(ctx):
	ctx.stlib(
		source=ctx.path.ant_glob(ctx.env.FILES_CSP),
		target = 'csp',
		includes= ctx.env.INCLUDES_CSP,
		export_includes = 'include',
		cflags = ctx.env.CFLAGS_CSP,
		defines = ctx.env.DEFINES_CSP,
		install_path = ctx.options.libdir,
		use = 'csp_size')
        if ctx.options.verbose > 0:
                ctx(rule='${SIZE} --format=berkeley ${SRC}', source='libcsp.a', name='csp_size', always=True)
	if ctx.options.enable_bindings:
		ctx.shlib(source=ctx.path.ant_glob(ctx.env.FILES_CSP),
			target = 'pycsp',
			includes= ctx.env.INCLUDES_CSP,
			export_includes = 'include',
			cflags = ctx.env.CFLAGS_CSP,
			defines = ctx.env.DEFINES_CSP,
			lib=['rt', 'pthread'])

def dist(ctx):
	ctx.excl = 'build/* **/.* **/*.pyc **/*.o **/*~ *.tar.gz'

