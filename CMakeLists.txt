cmake_minimum_required(VERSION 3.20)
project(csp VERSION 2.1)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(BUILD_SHARED_LIBS ON)
endif()

add_library(${PROJECT_NAME})
# This allows users which use the add_subdirectory or FetchContent
# to use the same target as users which use find_package
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 11)
set_target_properties(${PROJECT_NAME} PROPERTIES C_EXTENSIONS ON)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

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
option(CSP_USE_RTABLE "Use routing table" OFF)

option(CSP_ENABLE_PYTHON3_BINDINGS "Build Python3 binding" OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CSP_POSIX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Zephyr")
  set(CSP_ZEPHYR 1)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
    CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")

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
  else()
    message(NOTICE "No pkg-config found")
  endif()
endif()

file(REAL_PATH include csp_inc)
file(REAL_PATH src csp_inc_src)
list(APPEND csp_inc ${CMAKE_CURRENT_BINARY_DIR}/include)
target_include_directories(csp PRIVATE ${csp_inc_src})

if(CSP_POSIX)
  set(CSP_C_ARGS -Wshadow -Wcast-align -Wpointer-arith -Wwrite-strings -Wno-unused-parameter)
elseif(CSP_ZEPHYR)
  set(CSP_C_ARGS -Wwrite-strings -Wno-unused-parameter)
endif()
target_compile_options(csp PRIVATE ${CSP_C_ARGS})

add_subdirectory(src)
add_subdirectory(examples)

if(${CSP_ENABLE_PYTHON3_BINDINGS})
  find_package(Python3 COMPONENTS Development.Module)
  if(Python3_Development.Module_FOUND)
    Python3_add_library(libcsp_py3 MODULE WITH_SOABI src/bindings/python/pycsp.c)
    target_include_directories(libcsp_py3 PUBLIC ${csp_inc})
    target_link_libraries(libcsp_py3 PUBLIC csp)
  else()
    message(FATAL_ERROR "Python development module not found")
  endif()
endif()

configure_file(csp_autoconfig.h.in include/csp/autoconfig.h)

include(GNUInstallDirs)

# We need different include directories depending on whether the library is being built
# or whether the library is used from an installed location.
# See https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html#exporting-targets for
# more information.
target_include_directories(${PROJECT_NAME}
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(${PROJECT_NAME} INTERFACE)

# only install if top level project
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Zephyr" AND ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  # If you want to know the specific or what is done here and why:
  # https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html#id1
  include(CMakePackageConfigHelpers)
  install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  # Generate the package configuration files using CMake provided macros
  write_basic_package_version_file(
    "${PROJECT_NAME}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
  )
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${PROJECT_NAME}
  )

  # Install target file
  install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
  # Install package configuration files.
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )

  # Install header files.
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/csp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(DIRECTORY ${PROJECT_BINARY_DIR}/include/csp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
