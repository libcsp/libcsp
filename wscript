#! /usr/bin/env python
# encoding: utf-8

import os

APPNAME = 'libcsp'
VERSION = '1.0'

top = '.'
out = 'build'

def options(ctx):
    ctx.load('gcc')
    ctx.add_option('--toolchain', default='', help='Set toolchain prefix')
    ctx.add_option('--conf-kernel', default=None, help='Set configuration file')
    ctx.add_option('--arch', default='posix', help='Set architecture. Must be either \'posix\' or \'freertos\'')
    ctx.add_option('--with-can', default=None, metavar='CHIP', help='Enable CAN driver. CHIP must be either socketcan, at91sam7a1, at91sam7a3 or at90can128')
    ctx.add_option('--cflags', default='', help='Add additional CFLAGS. Separate with comma')

def configure(ctx):
    if not ctx.options.arch in ('posix', 'freertos'):
        ctx.fatal('ARCH must be either \'posix\' or \'freertos\'')

    if not ctx.options.with_can in (None, 'socketcan', 'at91sam7a1', 'at91sam7a3', 'at90can128'):
        ctx.fatal('CAN must be either \'socketcan\', \'at91sam7a1\', \'at91sam7a3\', \'at90can128\'')

    ctx.env.CC = ctx.options.toolchain + "gcc"
    ctx.env.AR = ctx.options.toolchain + "ar"
    ctx.load('gcc')

    ctx.env.FILES_CSP = [
            'src/*.c', 
            'src/crypto/*.c',
            'src/interfaces/csp_if_lo.c',
            'src/transport/*.c',
            'src/{0}/**/*.c'.format(ctx.options.arch)
            ]
    ctx.env.INCLUDES_GOMSPACE = ['../../libio/include', '../../libcdh/include']

    if ctx.options.arch == 'freertos':
        ctx.env.INCLUDES_GOMSPACE.append('../../libgomspace/include')

    if ctx.options.with_can:
        ctx.env.FILES_CSP.append('src/interfaces/csp_if_can.c')
    if ctx.options.with_can == 'socketcan':
        ctx.env.FILES_GOMSPACE.append('src/interfaces/can/can_socketcan.c')
    elif ctx.options.with_can == 'at91sam7a1':
        ctx.env.FILES_GOMSPACE.append('src/interfaces/can/can_at91sam7a1.c')
    elif ctx.options.with_can == 'at91sam7a3':
        ctx.env.FILES_GOMSPACE.append('src/interfaces/can/can_at91sam7a3.c')
    elif ctx.options.with_can == 'at90can128':
        ctx.env.FILES_GOMSPACE.append('src/interfaces/can/can_at90can128.c')

    if ctx.options.conf_kernel:
        ctx.define('CONF_KERNEL', os.path.abspath(ctx.options.conf_kernel))

    if ctx.options.verbose > 0:
        print(ctx.options)
        print(type(ctx.env))
        print(ctx.env)

def build(ctx):
    ctx.stlib(
            source=ctx.path.ant_glob(ctx.env.FILES_CSP),
            target='csp',
            includes='include',
            export_includes='include', 
            use='freertos drivers GOMSPACE',
            cflags = ['-O3','-Wall', '-g'] + ctx.options.cflags.split(',')
            )
