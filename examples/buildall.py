#!/usr/bin/env python
# encoding: utf-8

import subprocess
import sys
import argparse


DEFAULT_BUILD_SYSTEM = 'waf'


def build_with_meson():
    extra_target = ['examples/csp_server_client',
                    'examples/csp_arch',
                    'examples/zmqproxy']
    builddir = 'build'

    meson_setup = ['meson', 'setup', builddir]
    meson_compile = ['ninja', '-C', builddir]
    subprocess.check_call(meson_setup)
    subprocess.check_call(meson_compile + extra_target)


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
            '--enable-python3-bindings',
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


def main(with_waf, options):
    if (with_waf):
        build_with_waf(options)
    else:
        build_with_meson()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-system',
                        default=DEFAULT_BUILD_SYSTEM,
                        choices=['meson', 'waf'])
    args, rest = parser.parse_known_args()

    main(args.build_system == DEFAULT_BUILD_SYSTEM, rest)
