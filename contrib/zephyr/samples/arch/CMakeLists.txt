# SPDX-License-Identifier: Apache-2.0

project(csp_arch)
cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
target_sources(app PRIVATE main.c)
target_link_libraries(app PRIVATE libcsp)
