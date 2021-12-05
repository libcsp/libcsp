#!/usr/bin/env python
# encoding: utf-8

import subprocess
import sys
import argparse


DEFAULT_BUILD_SYSTEM = 'waf'


def build_with_meson():
    targets = ['examples/csp_server_client',
               'examples/csp_arch',
               #'examples/csp_arch_shared',
               'examples/zmqproxy']
    builddir = 'build'

    meson_setup = ['meson', 'setup', builddir]
    meson_compile = ['ninja', '-C', builddir]
    subprocess.check_call(meson_setup)
    subprocess.check_call(meson_compile + targets)


def build_with_cmake():
    targets = ['examples/csp_server_client',
               'examples/csp_arch',
               'examples/zmqproxy']
    builddir = 'build'

    cmake_setup = ['cmake', '-GNinja', '-B' + builddir]
    cmake_compile = ['ninja', '-C', builddir]
    subprocess.check_call(cmake_setup)
    subprocess.check_call(cmake_compile + targets)


def build_with_waf(options):
    target_os = 'posix'  # default OS
    if (len(options) > 0) and not options[0].startswith('--'):
        target_os = options[0]
        options = options[1:]

    options += [
        '--with-os=' + target_os,
        '--enable-rdp',
        '--enable-promisc',
        '--enable-crc32',
        '--enable-hmac',
        '--enable-xtea',
        '--enable-dedup',
        '--with-loglevel=debug',
        '--enable-debug-timestamp'
    ]

    waf = ['./waf']
    if target_os in ['posix']:
        options += [
            '--enable-can-socketcan',
            '--with-driver-usart=linux',
            '--enable-if-zmqhub',
            '--enable-shlib'
        ]

    if target_os in ['macosx']:
        options += [
            '--with-driver-usart=linux',
        ]

    if target_os in ['windows']:
        options += [
            '--with-driver-usart=windows',
        ]
        waf = ['python', '-x', 'waf']

    # Build
    waf += ['distclean', 'configure', 'build']
    print("Waf build command:", waf)
    subprocess.check_call(waf + options +
                          ['--with-rtable=cidr', '--disable-stlib', '--disable-output'])
    subprocess.check_call(waf + options + ['--enable-examples'])


def main(build_system, options):
    if (build_system == 'waf'):
        build_with_waf(options)
    elif (build_system == 'cmake'):
        build_with_cmake()
    else:
        build_with_meson()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-system',
                        default=DEFAULT_BUILD_SYSTEM,
                        choices=['meson', 'cmake', 'waf'])
    args, rest = parser.parse_known_args()

    main(args.build_system, rest)
