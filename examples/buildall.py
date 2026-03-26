#!/usr/bin/env python
# encoding: utf-8

import subprocess
import argparse


DEFAULT_BUILD_SYSTEM = 'waf'


def build_with_meson():
    targets = ['examples/csp_server_client',
               'examples/csp_server',
               'examples/csp_client',
               'examples/csp_bridge_can2udp',
               'examples/csp_arch',
               'examples/zmqproxy']
    builddir = 'build'

    meson_setup = ['meson', 'setup', builddir]
    meson_compile = ['ninja', '-C', builddir]
    subprocess.check_call(meson_setup)
    subprocess.check_call(meson_compile + targets)


def build_with_cmake():
    build_samples = '-DCSP_BUILD_SAMPLES=ON'
    builddir = 'build'

    cmake_setup = ['cmake', '-GNinja', '-B' + builddir, build_samples]
    cmake_compile = ['ninja', '-C', builddir]
    subprocess.check_call(cmake_setup)
    subprocess.check_call(cmake_compile)


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
        '--enable-yaml',
    ]

    subprocess.check_call(['./waf', 'distclean', 'configure', 'build'])
    subprocess.check_call(['./waf', 'distclean', 'configure', 'build'] + options)


def main(build_system):
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
    args = parser.parse_args()

    main(args.build_system)
