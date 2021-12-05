#!/usr/bin/env python
# encoding: utf-8

import os

APPNAME = 'libcsp'
VERSION = '2.0'

valid_os = ['posix', 'freertos']
valid_loglevel = ['none', 'error', 'warn', 'info', 'debug']


def options(ctx):
    # Load compiler
    ctx.load('compiler_c')

    ctx.add_option('--toolchain', default=None, help='Set toolchain prefix')

    # Set libcsp options
    gr = ctx.add_option_group('libcsp options')
    gr.add_option('--includes', default='', help='Add additional include paths, separate with comma')
    gr.add_option('--install-csp', action='store_true', help='Installs CSP headers and lib')

    gr.add_option('--disable-output', action='store_true', help='Disable CSP output')
    gr.add_option('--disable-stlib', action='store_true', help='Build objects only')
    gr.add_option('--enable-shlib', action='store_true', help='Build shared library')
    gr.add_option('--enable-rdp', action='store_true', help='Enable RDP support')
    gr.add_option('--enable-rdp-fast-close', action='store_true', help='Enable fast close of RDP connections')
    gr.add_option('--enable-promisc', action='store_true', help='Enable promiscuous support')
    gr.add_option('--enable-crc32', action='store_true', help='Enable CRC32 support')
    gr.add_option('--enable-hmac', action='store_true', help='Enable HMAC-SHA1 support')
    gr.add_option('--enable-xtea', action='store_true', help='Enable XTEA support')
    gr.add_option('--enable-python3-bindings', action='store_true', help='Enable Python3 bindings')
    gr.add_option('--enable-examples', action='store_true', help='Enable examples')
    gr.add_option('--enable-dedup', action='store_true', help='Enable packet deduplicator')
    gr.add_option('--enable-external-debug', action='store_true', help='Enable external debug API')
    gr.add_option('--enable-debug-timestamp', action='store_true', help='Enable timestamps on debug/log')

    # Drivers and interfaces (requires external dependencies)
    gr.add_option('--enable-if-zmqhub', action='store_true', help='Enable ZMQ interface')
    gr.add_option('--enable-can-socketcan', action='store_true', help='Enable Linux socketcan driver')
    gr.add_option('--with-driver-usart', default=None, metavar='DRIVER', help='Build USART driver. [linux, None]')

    # OS
    gr.add_option('--with-os', metavar='OS', default='posix', help='Set operating system. Must be one of: ' + str(valid_os))

    # Options
    gr.add_option('--with-loglevel', metavar='LEVEL', default='debug', help='Set minimum compile time log level. Must be one of: ' + str(valid_loglevel))
    gr.add_option('--with-rtable', metavar='TABLE', default='cidr', help='Set routing table type: \'static\' or \'cidr\'')


def configure(ctx):
    # Validate options
    if ctx.options.with_os not in valid_os:
        ctx.fatal('--with-os must be either: ' + str(valid_os))

    if ctx.options.with_loglevel not in valid_loglevel:
        ctx.fatal('--with-loglevel must be either: ' + str(valid_loglevel))

    # Setup and validate toolchain
    if (len(ctx.stack_path) <= 1) and ctx.options.toolchain:
        ctx.env.CC = ctx.options.toolchain + 'gcc'
        ctx.env.AR = ctx.options.toolchain + 'ar'

    ctx.load('compiler_c python')

    # Set git revision define
    git_rev = os.popen('git describe --long --always 2> /dev/null || echo unknown').read().strip()
    ctx.define('GIT_REV', git_rev)

    # Set build output format
    ctx.env.FEATURES = ['c']
    if not ctx.options.disable_stlib:
        ctx.env.FEATURES += ['cstlib']

    ctx.env.LIBCSP_SHLIB = ctx.options.enable_shlib

    # Setup CFLAGS
    if (len(ctx.stack_path) <= 1) and (len(ctx.env.CFLAGS) == 0):
        ctx.env.prepend_value('CFLAGS', ["-std=gnu11", "-g", "-Os", "-Wall", "-Wextra", "-Wshadow", "-Wcast-align",
                                         "-Wwrite-strings", "-Wno-unused-parameter", "-Werror"])

    # Setup default include path and any extra defined
    ctx.env.append_unique('INCLUDES_CSP', ['.', 'include'] + ctx.options.includes.split(','))

    # Store OS as env variable
    ctx.env.OS = ctx.options.with_os

    # Platform/OS specifics
    if ctx.options.with_os == 'posix':
        ctx.env.append_unique('LIBS', ['rt', 'pthread', 'util'])
    elif ctx.options.with_os == 'macosx':
        ctx.env.append_unique('LIBS', ['pthread'])
        if ctx.env.CC_VERSION[0] == '9':
            ctx.env.append_unique('CFLAGS', ['-Wno-missing-field-initializers'])

    ctx.define_cond('CSP_FREERTOS', ctx.options.with_os == 'freertos')
    ctx.define_cond('CSP_POSIX', ctx.options.with_os == 'posix')


    # Add files
    ctx.env.append_unique('FILES_CSP', ['src/crypto/csp_hmac.c',
                                        'src/crypto/csp_sha1.c',
                                        'src/crypto/csp_xtea.c',
                                        'src/transport/csp_rdp.c',
                                        'src/transport/csp_rdp_queue.c',
                                        'src/transport/csp_udp.c',
                                        'src/csp_buffer.c',
                                        'src/csp_bridge.c',
                                        'src/csp_conn.c',
                                        'src/csp_crc32.c',
                                        'src/csp_debug.c',
                                        'src/csp_dedup.c',
                                        'src/csp_hex_dump.c',
                                        'src/csp_iflist.c',
                                        'src/csp_init.c',
                                        'src/csp_io.c',
                                        'src/csp_port.c',
                                        'src/csp_promisc.c',
                                        'src/csp_qfifo.c',
                                        'src/csp_route.c',
                                        'src/csp_service_handler.c',
                                        'src/csp_services.c',
                                        'src/csp_id.c',
                                        'src/csp_sfp.c',                                       
                                        'src/interfaces/csp_if_lo.c',
                                        'src/interfaces/csp_if_can.c',
                                        'src/interfaces/csp_if_can_pbuf.c',
                                        'src/interfaces/csp_if_kiss.c',
                                        'src/arch/*.c',
                                        'src/arch/{0}/**/*.c'.format(ctx.options.with_os),
                                        'src/csp_rtable_stdio.c',
                                        'src/csp_rtable_cidr.c'])

    # Add if UDP
    if ctx.check(header_name="sys/socket.h", mandatory=False) and ctx.check(header_name="arpa/inet.h", mandatory=False):
        ctx.env.append_unique('FILES_CSP', ['src/interfaces/csp_if_udp.c'])

    # Add socketcan
    if ctx.options.enable_can_socketcan:
        ctx.env.append_unique('FILES_CSP', 'src/drivers/can/can_socketcan.c')
        ctx.check_cfg(package='libsocketcan', args='--cflags --libs', define_name='CSP_HAVE_LIBSOCKETCAN')
        ctx.env.append_unique('LIBS', ctx.env.LIB_LIBSOCKETCAN)

    # Add USART driver
    if ctx.options.with_driver_usart:
        ctx.env.append_unique('FILES_CSP', ['src/drivers/usart/usart_kiss.c',
                                            'src/drivers/usart/usart_{0}.c'.format(ctx.options.with_driver_usart)])

    # Add ZMQ
    if ctx.options.enable_if_zmqhub:
        ctx.check_cfg(package='libzmq', args='--cflags --libs', define_name='CSP_HAVE_LIBZMQ')
        ctx.env.append_unique('LIBS', ctx.env.LIB_LIBZMQ)
        ctx.env.append_unique('FILES_CSP', 'src/interfaces/csp_if_zmqhub.c')

    # Store configuration options
    ctx.env.ENABLE_EXAMPLES = ctx.options.enable_examples

    # Add Python bindings
    if ctx.options.enable_python3_bindings:
        ctx.check_python_version((3,5))
        ctx.check_python_headers(features='pyext')

    # Set defines for customizable parameters
    ctx.define('CSP_QFIFO_LEN', 15)
    ctx.define('CSP_PORT_MAX_BIND', 16)
    ctx.define('CSP_CONN_RXQUEUE_LEN', 15)
    ctx.define('CSP_CONN_MAX', 8)
    ctx.define('CSP_BUFFER_SIZE', 256)
    ctx.define('CSP_BUFFER_COUNT', 15)
    ctx.define('CSP_RDP_MAX_WINDOW', 5)
    ctx.define('CSP_RTABLE_SIZE', 10)

    # Set defines for enabling features
    ctx.define('CSP_HAVE_STDIO', not ctx.options.disable_output)
    ctx.define('CSP_USE_RDP', ctx.options.enable_rdp)
    ctx.define('CSP_USE_RDP_FAST_CLOSE', ctx.options.enable_rdp and ctx.options.enable_rdp_fast_close)
    ctx.define('CSP_USE_HMAC', ctx.options.enable_hmac)
    ctx.define('CSP_USE_XTEA', ctx.options.enable_xtea)
    ctx.define('CSP_USE_PROMISC', ctx.options.enable_promisc)
    ctx.define('CSP_USE_DEDUP', ctx.options.enable_dedup)
    ctx.define('CSP_USE_EXTERNAL_DEBUG', ctx.options.enable_external_debug)


    ctx.write_config_header('csp_autoconfig.h')

def build(ctx):

    # Set install path for header files
    install_path = None
    if ctx.options.install_csp:
        install_path = '${PREFIX}/lib'
        ctx.install_files('${PREFIX}/include/csp',
                          ctx.path.ant_glob('include/csp/**'),
                          cwd=ctx.path.find_dir('include/csp'),
                          relative_trick=True)
        ctx.install_files('${PREFIX}/include/csp', 'csp_autoconfig.h', cwd=ctx.bldnode)

    ctx(export_includes=ctx.env.INCLUDES_CSP, name='csp_h')

    ctx(features=ctx.env.FEATURES,
        source=ctx.path.ant_glob(ctx.env.FILES_CSP),
        target='csp',
        use=['csp_h', 'freertos_h', 'util'],
        install_path=install_path)

    # Build shared library
    if ctx.env.LIBCSP_SHLIB or ctx.env.HAVE_PYEXT:
        ctx.shlib(source=ctx.path.ant_glob(ctx.env.FILES_CSP),
                  name='csp_shlib',
                  target='csp',
                  use=['csp_h', 'util_shlib'],
                  lib=ctx.env.LIBS)

    # Build Python bindings
    if ctx.env.HAVE_PYEXT:
        ctx.shlib(source=ctx.path.ant_glob('src/bindings/python/**/*.c'),
                  target='libcsp_py3',
                  features='pyext',
                  use=['csp_shlib'],
                  pytest_path=[ctx.path.get_bld()])

    if ctx.env.ENABLE_EXAMPLES:
        ctx.program(source=['examples/csp_server_client.c',
                            'examples/csp_server_client_{0}.c'.format(ctx.env.OS)],
                    target='examples/csp_server_client',
                    lib=ctx.env.LIBS,
                    use='csp')

        ctx.program(source='examples/csp_arch.c',
                    target='examples/csp_arch',
                    lib=ctx.env.LIBS,
                    use='csp')

        if ctx.env.CSP_HAVE_LIBZMQ:
            ctx.program(source='examples/zmqproxy.c',
                        target='examples/zmqproxy',
                        lib=ctx.env.LIBS,
                        use='csp')


def dist(ctx):
    ctx.excl = 'build/* **/.* **/*.pyc **/*.o **/*~ *.tar.gz'
