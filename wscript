#!/usr/bin/env python
# encoding: utf-8

# Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
# Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
# Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk) 
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
VERSION = '1.4'

top = '.'
out = 'build'

def options(ctx):
    # Load GCC options
    ctx.load('gcc')
    
    ctx.add_option('--toolchain', default=None, help='Set toolchain prefix')

    # Set libcsp options
    gr = ctx.add_option_group('libcsp options')
    gr.add_option('--includes', default='', help='Add additional include paths. Separate with comma')
    gr.add_option('--install-csp', action='store_true', help='Installs CSP headers and lib')

    gr.add_option('--disable-output', action='store_true', help='Disable CSP output')
    gr.add_option('--disable-stlib', action='store_true', help='Build objects only')
    gr.add_option('--enable-rdp', action='store_true', help='Enable RDP support')
    gr.add_option('--enable-qos', action='store_true', help='Enable Quality of Service support')
    gr.add_option('--enable-promisc', action='store_true', help='Enable promiscuous mode support')
    gr.add_option('--enable-crc32', action='store_true', help='Enable CRC32 support')
    gr.add_option('--enable-hmac', action='store_true', help='Enable HMAC-SHA1 support')
    gr.add_option('--enable-xtea', action='store_true', help='Enable XTEA support')
    gr.add_option('--enable-bindings', action='store_true', help='Enable Python bindings')
    gr.add_option('--enable-examples', action='store_true', help='Enable examples')
    gr.add_option('--enable-dedup', action='store_true', help='Enable packet deduplicator')

    # Interfaces    
    gr.add_option('--enable-if-i2c', action='store_true', help='Enable I2C interface')
    gr.add_option('--enable-if-kiss', action='store_true', help='Enable KISS/RS.232 interface')
    gr.add_option('--enable-if-can', action='store_true', help='Enable CAN interface')
    gr.add_option('--enable-if-zmqhub', action='store_true', help='Enable ZMQHUB interface')
    
    # Drivers
    gr.add_option('--enable-can-socketcan', default=None, metavar='CHIP', help='Enable Linux socketcan driver')
    gr.add_option('--with-driver-usart', default=None, metavar='DRIVER', help='Build USART driver. [windows, linux, None]')

    # OS    
    gr.add_option('--with-os', metavar='OS', default='posix', help='Set operating system. Must be either \'posix\', \'macosx\', \'windows\' or \'freertos\'')
    gr.add_option('--enable-init-shutdown', action='store_true', help='Use init system commands for shutdown/reboot')

    # Options
    gr.add_option('--with-rdp-max-window', metavar='SIZE', type=int, default=20, help='Set maximum window size for RDP')
    gr.add_option('--with-max-bind-port', metavar='PORT', type=int, default=31, help='Set maximum bindable port')
    gr.add_option('--with-max-connections', metavar='COUNT', type=int, default=10, help='Set maximum number of concurrent connections')
    gr.add_option('--with-conn-queue-length', metavar='SIZE', type=int, default=100, help='Set maximum number of packets in queue for a connection')
    gr.add_option('--with-router-queue-length', metavar='SIZE', type=int, default=10, help='Set maximum number of packets to be queued at the input of the router')
    gr.add_option('--with-padding', metavar='BYTES', type=int, default=8, help='Set padding bytes before packet length field')
    gr.add_option('--with-loglevel', metavar='LEVEL', default='debug', help='Set minimum compile time log level. Must be one of \'error\', \'warn\', \'info\' or \'debug\'')
    gr.add_option('--with-rtable', metavar='TABLE', default='static', help='Set routing table type')
    gr.add_option('--with-connection-so', metavar='CSP_SO', type=int, default='0x0000', help='Set outgoing connection socket options, see csp.h for valid values')
    gr.add_option('--with-bufalign', metavar='BYTES', type=int, help='Set buffer alignment')

def configure(ctx):
    # Validate OS
    if not ctx.options.with_os in ('posix', 'windows', 'freertos', 'macosx'):
        ctx.fatal('--with-os must be either \'posix\', \'windows\', \'macosx\' or \'freertos\'')

    # Validate USART drivers
    if not ctx.options.with_driver_usart in (None, 'windows', 'linux'):
        ctx.fatal('--with-driver-usart must be either \'windows\' or \'linux\'')

    if not ctx.options.with_loglevel in ('error', 'warn', 'info', 'debug'):
        ctx.fatal('--with-loglevel must be either \'error\', \'warn\', \'info\' or \'debug\'')

    # Setup and validate toolchain
    if ctx.options.toolchain:
        ctx.env.CC = ctx.options.toolchain + 'gcc'
        ctx.env.AR = ctx.options.toolchain + 'ar'

    ctx.load('gcc')

    # Set git revision define
    git_rev = os.popen('git describe --always 2> /dev/null || echo unknown').read().strip()

    # Setup DEFINES
    ctx.define('GIT_REV', git_rev)

    # Set build output format
    ctx.env.FEATURES = ['c']
    if not ctx.options.disable_stlib:
        ctx.env.FEATURES += ['cstlib']

    # Setup CFLAGS
    if (len(ctx.env.CFLAGS) == 0):
        ctx.env.prepend_value('CFLAGS', ['-Os','-Wall', '-g', '-std=gnu99'])

    # Setup extra includes
    ctx.env.append_unique('INCLUDES_CSP', ['include'] + ctx.options.includes.split(','))

    # Add default files
    ctx.env.append_unique('FILES_CSP', ['src/*.c','src/interfaces/csp_if_lo.c','src/transport/csp_udp.c','src/arch/{0}/**/*.c'.format(ctx.options.with_os)])
    
    # Store OS as env variable
    ctx.env.append_unique('OS', ctx.options.with_os)

    # Libs
    if 'posix' in ctx.env.OS:
        ctx.env.append_unique('LIBS', ['rt', 'pthread', 'util'])
    elif 'macosx' in ctx.env.OS:
        ctx.env.append_unique('LIBS', ['pthread'])

    # Check for recursion
    if ctx.path == ctx.srcnode:
        ctx.options.install_csp = True
    
    # Windows build flags
    if ctx.options.with_os == 'windows':
        ctx.env.append_unique('CFLAGS', ['-D_WIN32_WINNT=0x0600'])

    ctx.define_cond('CSP_FREERTOS', ctx.options.with_os == 'freertos')
    ctx.define_cond('CSP_POSIX', ctx.options.with_os == 'posix')
    ctx.define_cond('CSP_WINDOWS', ctx.options.with_os == 'windows')
    ctx.define_cond('CSP_MACOSX', ctx.options.with_os == 'macosx')
        
    # Add CAN driver
    if ctx.options.enable_can_socketcan:
        ctx.env.append_unique('FILES_CSP', 'src/drivers/can/can_socketcan.c')

    # Add USART driver
    if ctx.options.with_driver_usart != None:
        ctx.env.append_unique('FILES_CSP', 'src/drivers/usart/usart_{0}.c'.format(ctx.options.with_driver_usart))
        
    # Interfaces
    if ctx.options.enable_if_can:
        ctx.env.append_unique('FILES_CSP', 'src/interfaces/csp_if_can.c')
    if ctx.options.enable_if_i2c:
        ctx.env.append_unique('FILES_CSP', 'src/interfaces/csp_if_i2c.c')
    if ctx.options.enable_if_kiss:
        ctx.env.append_unique('FILES_CSP', 'src/interfaces/csp_if_kiss.c')
    if ctx.options.enable_if_zmqhub:
        ctx.env.append_unique('FILES_CSP', 'src/interfaces/csp_if_zmqhub.c')
        ctx.check_cfg(package='libzmq', args='--cflags --libs')
        ctx.env.append_unique('LIBS', ctx.env.LIB_LIBZMQ)

    # Store configuration options
    ctx.env.ENABLE_BINDINGS = ctx.options.enable_bindings
    ctx.env.ENABLE_EXAMPLES = ctx.options.enable_examples
    
    # Create config file
    if not ctx.options.disable_output:
        ctx.env.append_unique('FILES_CSP', 'src/csp_debug.c')
    else:
        ctx.env.append_unique('EXCL_CSP', 'src/csp_debug.c')

    if ctx.options.enable_rdp:
        ctx.env.append_unique('FILES_CSP', 'src/transport/csp_rdp.c')

    if ctx.options.enable_crc32:
        ctx.env.append_unique('FILES_CSP', 'src/csp_crc32.c')
    else:
        ctx.env.append_unique('EXCL_CSP', 'src/csp_crc32.c')

    if not ctx.options.enable_dedup:
        ctx.env.append_unique('EXCL_CSP', 'src/csp_dedup.c')

    if ctx.options.enable_hmac:
        ctx.env.append_unique('FILES_CSP', 'src/crypto/csp_hmac.c')
        ctx.env.append_unique('FILES_CSP', 'src/crypto/csp_sha1.c')

    if ctx.options.enable_xtea:
        ctx.env.append_unique('FILES_CSP', 'src/crypto/csp_xtea.c')
        ctx.env.append_unique('FILES_CSP', 'src/crypto/csp_sha1.c')
        
    ctx.env.append_unique('FILES_CSP', 'src/rtable/csp_rtable_' + ctx.options.with_rtable  + '.c')

    ctx.define_cond('CSP_DEBUG', not ctx.options.disable_output)
    ctx.define_cond('CSP_USE_RDP', ctx.options.enable_rdp)
    ctx.define_cond('CSP_USE_CRC32', ctx.options.enable_crc32)
    ctx.define_cond('CSP_USE_HMAC', ctx.options.enable_hmac)
    ctx.define_cond('CSP_USE_XTEA', ctx.options.enable_xtea)
    ctx.define_cond('CSP_USE_PROMISC', ctx.options.enable_promisc)
    ctx.define_cond('CSP_USE_QOS', ctx.options.enable_qos)
    ctx.define_cond('CSP_USE_DEDUP', ctx.options.enable_dedup)
    ctx.define_cond('CSP_USE_INIT_SHUTDOWN', ctx.options.enable_init_shutdown)
    ctx.define('CSP_CONN_MAX', ctx.options.with_max_connections)
    ctx.define('CSP_CONN_QUEUE_LENGTH', ctx.options.with_conn_queue_length)
    ctx.define('CSP_FIFO_INPUT', ctx.options.with_router_queue_length)
    ctx.define('CSP_MAX_BIND_PORT', ctx.options.with_max_bind_port)
    ctx.define('CSP_RDP_MAX_WINDOW', ctx.options.with_rdp_max_window)
    ctx.define('CSP_PADDING_BYTES', ctx.options.with_padding)
    ctx.define('CSP_CONNECTION_SO', ctx.options.with_connection_so)
    
    if ctx.options.with_bufalign != None:
        ctx.define('CSP_BUFFER_ALIGN', ctx.options.with_bufalign)

    # Set logging level
    ctx.define_cond('CSP_LOG_LEVEL_DEBUG', ctx.options.with_loglevel in ('debug'))
    ctx.define_cond('CSP_LOG_LEVEL_INFO', ctx.options.with_loglevel in ('debug', 'info'))
    ctx.define_cond('CSP_LOG_LEVEL_WARN', ctx.options.with_loglevel in ('debug', 'info', 'warn'))
    ctx.define_cond('CSP_LOG_LEVEL_ERROR', ctx.options.with_loglevel in ('debug', 'info', 'warn', 'error'))

    # Check compiler endianness
    endianness = ctx.check_endianness()
    ctx.define_cond('CSP_LITTLE_ENDIAN', endianness == 'little')
    ctx.define_cond('CSP_BIG_ENDIAN', endianness == 'big')

    # Check for stdbool.h
    ctx.check_cc(header_name='stdbool.h', mandatory=False, define_name='CSP_HAVE_STDBOOL_H', type='cstlib')

    # Check for libsocketcan.h
    if ctx.options.enable_if_can and ctx.options.enable_can_socketcan:
        have_socketcan = ctx.check_cc(lib='socketcan', mandatory=False, define_name='CSP_HAVE_LIBSOCKETCAN')
        if have_socketcan:
            ctx.env.append_unique('LIBS', ['socketcan'])

    ctx.define('LIBCSP_VERSION', VERSION)

    ctx.write_config_header('include/csp/csp_autoconfig.h', top=True, remove=True)
    
def build(ctx):

    # Set install path for header files
    install_path = False
    if ctx.options.install_csp:
        install_path = '${PREFIX}/lib'
        ctx.install_files('${PREFIX}/include/csp', ctx.path.ant_glob('include/csp/*.h'))
        ctx.install_files('${PREFIX}/include/csp/interfaces', 'include/csp/interfaces/csp_if_lo.h')

        if 'src/interfaces/csp_if_can.c' in ctx.env.FILES_CSP:
            ctx.install_files('${PREFIX}/include/csp/interfaces', 'include/csp/interfaces/csp_if_can.h')
        if 'src/interfaces/csp_if_i2c.c' in ctx.env.FILES_CSP:
            ctx.install_files('${PREFIX}/include/csp/interfaces', 'include/csp/interfaces/csp_if_i2c.h')
        if 'src/interfaces/csp_if_kiss.c' in ctx.env.FILES_CSP:
            ctx.install_files('${PREFIX}/include/csp/interfaces', 'include/csp/interfaces/csp_if_kiss.h')
        if 'src/interfaces/csp_if_zmqhub.c' in ctx.env.FILES_CSP:
            ctx.install_files('${PREFIX}/include/csp/interfaces', 'include/csp/interfaces/csp_if_zmqhub.h')
        if 'src/drivers/usart/usart_{0}.c'.format(ctx.options.with_driver_usart) in ctx.env.FILES_CSP:
            ctx.install_as('${PREFIX}/include/csp/drivers/usart.h', 'include/csp/drivers/usart.h')

        ctx.install_files('${PREFIX}/include/csp', 'include/csp/csp_autoconfig.h', cwd=ctx.bldnode)

    ctx(export_includes='include', name='csp_h')

    ctx(features=ctx.env.FEATURES,
        source=ctx.path.ant_glob(ctx.env.FILES_CSP, excl=ctx.env.EXCL_CSP),
        target = 'csp',
        includes= ctx.env.INCLUDES_CSP,
        export_includes = ctx.env.INCLUDES_CSP,
        use = 'include freertos_h',
        install_path = install_path,
    )

    # Build shared library for Python bindings
    if ctx.env.ENABLE_BINDINGS:
        ctx.shlib(source=ctx.path.ant_glob(ctx.env.FILES_CSP),
            target = 'csp',
            includes= ctx.env.INCLUDES_CSP,
            export_includes = 'include',
            use = ['include'],
            lib = ctx.env.LIBS)

    if ctx.env.ENABLE_EXAMPLES:
        ctx.program(source = ctx.path.ant_glob('examples/simple.c'),
            target = 'simple',
            includes = ctx.env.INCLUDES_CSP,
            lib = ctx.env.LIBS,
            use = 'csp')

        if ctx.options.enable_if_kiss:
            ctx.program(source = 'examples/kiss.c',
                target = 'kiss',
                includes = ctx.env.INCLUDES_CSP,
                lib = ctx.env.LIBS,
                use = 'csp')

        if 'posix' in ctx.env.OS:
            ctx.program(source = 'examples/csp_if_fifo.c',
                target = 'fifo',
                includes = ctx.env.INCLUDES_CSP,
                lib = ctx.env.LIBS,
                use = 'csp')

        if 'windows' in ctx.env.OS:
            ctx.program(source = ctx.path.ant_glob('examples/csp_if_fifo_windows.c'),
                target = 'csp_if_fifo',
                includes = ctx.env.INCLUDES_CSP,
                use = 'csp')

def dist(ctx):
    ctx.excl = 'build/* **/.* **/*.pyc **/*.o **/*~ *.tar.gz'
