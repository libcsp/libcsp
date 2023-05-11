# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(csp_arch)

target_sources(app PRIVATE ../../../examples/csp_arch.c)
target_link_libraries(app PRIVATE csp)
