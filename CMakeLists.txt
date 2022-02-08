project(CSP)
cmake_minimum_required(VERSION 3.20)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(BUILD_SHARED_LIBS ON)
endif()

add_library(libcsp)
set_target_properties(libcsp PROPERTIES C_STANDARD 11)
set_target_properties(libcsp PROPERTIES C_EXTENSIONS ON)
target_compile_options(libcsp PRIVATE -Wall -Wextra)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to MinSizeRel")
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build Type" FORCE)
endif()

set(CSP_HAVE_STDIO 1)
set(CSP_ENABLE_CSP_PRINT 1)
set(CSP_PRINT_STDIO 0)
set(CSP_QFIFO_LEN 15 CACHE STRING "Length of incoming queue for router task.")
set(CSP_PORT_MAX_BIND 16 CACHE STRING "Length of incoming queue for router task")
set(CSP_CONN_RXQUEUE_LEN 16 CACHE STRING "Number of packets in connection queue")
set(CSP_CONN_MAX 8 CACHE STRING "Number of new connections on socket queue")
set(CSP_BUFFER_SIZE 256 CACHE STRING "Bytes in each packet buffer")
set(CSP_BUFFER_COUNT 15 CACHE STRING "Number of total packet buffers")
set(CSP_RDP_MAX_WINDOW 5 CACHE STRING "Max window size for RDP")
set(CSP_RTABLE_SIZE 10 CACHE STRING "Number of elements in routing table")

option(CSP_USE_RDP "Reliable Datagram Protocol" ON)
option(CSP_USE_HMAC "Hash-based message authentication code" ON)
option(CSP_USE_PROMISC "Promiscious mode" ON)
option(CSP_USE_DEDUP "Packet deduplication" ON)

option(enable-python3-bindings "Build Python3 binding")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CSP_POSIX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Zephyr")
  set(CSP_ZEPHYR 1)
endif()

include(CheckIncludeFiles)
check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_files(arpa/inet.h HAVE_ARPA_INET_H)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
    CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  find_package(Threads REQUIRED)
  find_package(PkgConfig)

  pkg_search_module(LIBZMQ libzmq)
  pkg_search_module(LIBSOCKETCAN libsocketcan)
endif()

file(REAL_PATH include csp_inc)
list(APPEND csp_inc ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(libcsp PUBLIC ${csp_inc})

if(CSP_POSIX)
  set(CSP_C_ARGS -Wshadow -Wcast-align -Wwrite-strings -Wno-unused-parameter)
elseif(CSP_ZEPHYR)
  set(CSP_C_ARGS -Wwrite-strings -Wno-unused-parameter)
endif()
target_compile_options(libcsp PRIVATE ${CSP_C_ARGS})

add_subdirectory(src)
add_subdirectory(examples)

if(${enable-python3-bindings})
  find_package(Python COMPONENTS Development.Module)
  if(Python_Development.Module_FOUND)
    Python_add_library(libcsp_py3 src/bindings/python/pycsp.c WITH_SOABI)
    target_include_directories(libcsp_py3 PRIVATE ${csp_inc})
    target_link_libraries(libcsp_py3 PRIVATE libcsp)
  endif()
endif()

configure_file(csp_autoconfig.h.in csp_autoconfig.h)
