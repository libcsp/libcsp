#!/usr/bin/env python
# encoding: utf-8

import subprocess
import argparse


DEFAULT_BUILD_SYSTEM = 'waf'


def build_with_meson():
    targets = ['examples/csp_server_client',
               'examples/csp_arch',
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


def build_with_waf():

    options = [
        '--with-os=posix',
        '--enable-rdp',
        '--enable-promisc',
        '--enable-crc32',
        '--enable-hmac',
        '--enable-dedup',
        '--enable-python3-bindings',
        '--enable-can-socketcan',
        '--with-driver-usart=linux',
        '--enable-if-zmqhub',
        '--enable-examples',
    ]

    subprocess.check_call(['./waf', 'distclean', 'configure', 'build'])
    subprocess.check_call(['./waf', 'distclean', 'configure', 'build'] + options)


def main(build_system, rest):
    if (build_system == 'waf'):
        build_with_waf()
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
