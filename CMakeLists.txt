cmake_minimum_required(VERSION 3.20)
project(CSP VERSION 2.1)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(BUILD_SHARED_LIBS ON)
endif()

add_library(csp)
set_target_properties(csp PROPERTIES C_STANDARD 11)
set_target_properties(csp PROPERTIES C_EXTENSIONS ON)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to MinSizeRel")
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build Type" FORCE)
endif()

option(CSP_REPRODUCIBLE_BUILDS "Reproducible build" OFF)

option(CSP_HAVE_STDIO "OS provides C Standard I/O functions" ON)
option(CSP_ENABLE_CSP_PRINT "Enable csp_print() function" ON)
option(CSP_PRINT_STDIO "Use vprintf() for csp_print() function" ON)

set(CSP_QFIFO_LEN 15 CACHE STRING "Length of incoming queue for router task")
set(CSP_PORT_MAX_BIND 16 CACHE STRING "Maximum number of bindable ports")
set(CSP_CONN_RXQUEUE_LEN 16 CACHE STRING "Number of packets in connection queue")
set(CSP_CONN_MAX 8 CACHE STRING "Number of new connections on socket queue")
set(CSP_BUFFER_SIZE 256 CACHE STRING "Bytes in each packet buffer")
set(CSP_BUFFER_COUNT 15 CACHE STRING "Number of total packet buffers")
set(CSP_RDP_MAX_WINDOW 5 CACHE STRING "Max window size for RDP")
set(CSP_RTABLE_SIZE 10 CACHE STRING "Number of elements in routing table")

option(CSP_USE_RDP "Reliable Datagram Protocol" ON)
option(CSP_USE_HMAC "Hash-based message authentication code" ON)
option(CSP_USE_PROMISC "Promiscious mode" ON)
option(CSP_USE_RTABLE "Use routing table" OFF)
option(CSP_BUFFER_ZERO_CLEAR "Zero out the packet buffer upon allocation" ON)

option(CSP_ENABLE_PYTHON3_BINDINGS "Build Python3 binding" OFF)
option(CSP_BUILD_SAMPLES "Build samples and examples by default" OFF)

option(CSP_FIXUP_V1_ZMQ_LITTLE_ENDIAN "Use little-endian CSP ID for ZMQ with CSPv1" ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CSP_POSIX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
  set(CSP_POSIX 1)
  message(WARNING "CYGWIN detected: experimental port and ethernet not implemented")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic")
  string(TOLOWER "${CSP_SYSTEM_NAME}" csp_system_name)
  if(csp_system_name STREQUAL "freertos")
    set(CSP_FREERTOS 1)
  elseif(csp_system_name STREQUAL "zephyr")
    set(CSP_ZEPHYR 1)
  else()
    message(FATAL_ERROR "Not supported ${CSP_SYSTEM_NAME}")
  endif()
endif()

if(CSP_POSIX AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")

  include(CheckIncludeFiles)
  check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
  check_include_files(arpa/inet.h HAVE_ARPA_INET_H)

  find_package(Threads REQUIRED)
  find_package(PkgConfig)

  if(PKG_CONFIG_FOUND)
    pkg_search_module(LIBYAML yaml-0.1)
    if(${LIBYAML_FOUND})
      message(STATUS "Found ${LIBYAML_LINK_LIBRARIES} ${LIBYAML_VERSION}")
      set(CSP_HAVE_LIBYAML 1)
    else()
      message(NOTICE "No libyaml found")
    endif()

    pkg_search_module(LIBZMQ libzmq)
    if(${LIBZMQ_FOUND})
      message(STATUS "Found ${LIBZMQ_LINK_LIBRARIES} ${LIBZMQ_VERSION}")
      set(CSP_HAVE_LIBZMQ 1)
    else()
      message(NOTICE "No libzmq found")
    endif()

    pkg_search_module(LIBSOCKETCAN libsocketcan)
    if(${LIBSOCKETCAN_FOUND})
      message(STATUS "Found ${LIBSOCKETCAN_LINK_LIBRARIES} ${LIBSOCKETCAN_VERSION}")
      set(CSP_HAVE_LIBSOCKETCAN 1)
    else()
      message(NOTICE "No libsocketcan found")
    endif()

    pkg_search_module(CHECK check)
    if(${CHECK_FOUND})
      message(STATUS "Found ${CHECK_MODULE_NAME} ${CHECK_VERSION}")
      set(CSP_HAVE_CHECK 1)
    else()
      message(NOTICE "No libcheck found")
    endif()
  else()
    message(NOTICE "No pkg-config found")
  endif()
endif()

file(REAL_PATH include csp_inc)
file(REAL_PATH src csp_inc_src)
list(APPEND csp_inc ${CMAKE_CURRENT_BINARY_DIR}/include)
target_include_directories(csp
  PUBLIC ${csp_inc}
  PRIVATE ${csp_inc_src}
)

add_library(csp_common INTERFACE)
if(CSP_POSIX)
  target_compile_options(csp_common INTERFACE
    -Wall
    -Wcast-align
    -Werror
    -Wextra
    -Wpedantic
    -Wpointer-arith
    -Wshadow
    -Wwrite-strings
    $<$<C_COMPILER_ID:Clang>:-Wno-gnu-zero-variadic-macro-arguments>)
endif()
target_link_libraries(csp PRIVATE csp_common)

if(NOT CSP_BUILD_SAMPLES)
  set(CSP_SAMPLES_EXCLUDE "EXCLUDE_FROM_ALL")
endif()

add_subdirectory(src)
add_subdirectory(unittests)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
  add_subdirectory(examples)
  add_subdirectory(samples)
endif()

if(${CSP_ENABLE_PYTHON3_BINDINGS})
  find_package(Python3 COMPONENTS Development.Module)
  if(Python3_Development.Module_FOUND)
    Python3_add_library(libcsp_py3 MODULE WITH_SOABI src/bindings/python/pycsp.c)
    add_compile_options(-Wno-unused-parameter)
    target_include_directories(libcsp_py3 PUBLIC ${csp_inc})
    target_link_libraries(libcsp_py3 PUBLIC csp)
  else()
    message(FATAL_ERROR "Python development module not found")
  endif()
endif()

configure_file(csp_autoconfig.h.in include/csp/autoconfig.h)

if(NOT CSP_ZEPHYR)
  install(TARGETS csp LIBRARY COMPONENT runtime)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/;${CMAKE_CURRENT_SOURCE_DIR}/include/;
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h*"
  )
endif()
