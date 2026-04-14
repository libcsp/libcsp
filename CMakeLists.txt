cmake_minimum_required(VERSION 3.20)

project(csp_es LANGUAGES C)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# Check if a version was provided by a parent project
if(DEFINED SUBMODULE_EXTERNAL_LIBCSP_ES_VERSION)
    # We are being built by the super-project
    set(PROJECT_VERSION ${SUBMODULE_EXTERNAL_LIBCSP_ES_VERSION})
    # Parse version components for super-project builds
    string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
    list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
    list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
    list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)
    message(STATUS "libcsp_es: Using version provided by super-project: ${PROJECT_VERSION}")
else()
    # Standalone build: accept an externally provided version or use a placeholder.
    # Override with: cmake -DPROJECT_VERSION=x.y.z
    if(NOT DEFINED PROJECT_VERSION OR PROJECT_VERSION STREQUAL "")
        set(PROJECT_VERSION "0.0.0")
        message(STATUS "libcsp_es: No version provided, using placeholder 0.0.0")
    else()
        message(STATUS "libcsp_es: Using externally provided version: ${PROJECT_VERSION}")
    endif()
    string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
    list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
    list(LENGTH VERSION_LIST VERSION_LIST_LENGTH)
    if(VERSION_LIST_LENGTH GREATER 1)
        list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
    else()
        set(PROJECT_VERSION_MINOR "0")
    endif()
    if(VERSION_LIST_LENGTH GREATER 2)
        list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)
    else()
        set(PROJECT_VERSION_PATCH "0")
    endif()
endif()


# Handle installation and deployment
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
# Only include install_paths for Linux builds when pkg-config is available
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        include(install_paths)
        message(STATUS "pkg-config found, including install_paths.cmake")
    else()
        message(STATUS "pkg-config not found, skipping install_paths.cmake - using fallback install paths")
        # Provide fallback installation paths when install_paths.cmake is not available
        set(INSTALL_RUNTIME_DIR ${CMAKE_INSTALL_BINDIR})
        set(INSTALL_CONFIG_DIR  ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${PROJECT_NAME})
        set(INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR})
        set(INSTALL_ARCHIVE_DIR ${CMAKE_INSTALL_LIBDIR})
        set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
    endif()
endif()

# Fix RPATH issues with Ninja generator
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

# Golang-specific build option
option(GOLANG "Enable flags for Go (cgo) compatibility" OFF)
if(GOLANG)
    message(STATUS "Building for Go interop")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#    set(BUILD_SHARED_LIBS ON)
    if(GOLANG)
        # Golang-specific PIC for posix builds
        message(STATUS "Position Independent Code turned ON")
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    endif()
endif()

## Set namespace for runtime versioning before building the submodules
set(CMAKE_GIT_PROJECT_PREFIX "${PROJECT_NAME}_" CACHE INTERNAL "Submodule project prefix" FORCE)
add_library(csp_es)

# Add alias to handle package export/import and  reconfiguration later easily
add_library(csp_es::csp_es ALIAS csp_es)

set_target_properties(csp_es PROPERTIES C_STANDARD 11)
set_target_properties(csp_es PROPERTIES C_EXTENSIONS ON)

# Make the SO comply to the GNU standard requirements
set_target_properties(csp_es PROPERTIES
                              VERSION ${PROJECT_VERSION}
                            SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Add -fanalyzer gcc specific option
target_compile_options(csp_es PRIVATE
		-Wall -Wextra -Wpedantic
		$<$<C_COMPILER_ID:GNU>:-fanalyzer>
)

#
# Build options
#
option(CSP_REPRODUCIBLE_BUILDS "Reproducible build" OFF)
option(CSP_ENABLE_PYTHON3_BINDINGS "Build Python3 binding" OFF)
option(CSP_BUILD_SAMPLES "Build samples and examples by default" ON)

# Configuration options
#
set(CSP_HAVE_STDIO ON CACHE BOOL "OS provides C Standard I/O functions")
set(CSP_ENABLE_CSP_PRINT ON CACHE BOOL "Enable csp_print() function")

set(CSP_PORT_MAX_BIND 16 CACHE STRING "Maximum number of bindable ports")
if(GOLANG)
  set(CSP_PRINT_STDIO OFF CACHE BOOL "Use vprintf() for csp_print() function")
  set(CSP_BUFFER_SIZE 4096 CACHE STRING "Bytes in each packet buffer")
  set(CSP_BUFFER_COUNT 4096 CACHE STRING "Number of total packet buffers")
  set(CSP_QFIFO_LEN 8192 CACHE STRING "Length of incoming queue for router task")
  # 47 = max outgoing ports = (63 - CSP_PORT_MAX_BIND) for 6-bit port fields (CSP v1 and v2).
  # Values above this would cause source port wrap-around and silent truncation on the wire.
  set(CSP_CONN_MAX 47 CACHE STRING "Number of new connections on socket queue")
  set(CSP_CONN_RXQUEUE_LEN 32 CACHE STRING "Number of packets in connection queue")
else()
  set(CSP_PRINT_STDIO ON CACHE BOOL "Use vprintf() for csp_print() function")
  set(CSP_BUFFER_SIZE 256 CACHE STRING "Bytes in each packet buffer")
  set(CSP_BUFFER_COUNT 15 CACHE STRING "Number of total packet buffers")
  set(CSP_QFIFO_LEN 15 CACHE STRING "Length of incoming queue for router task")
  set(CSP_CONN_MAX 8 CACHE STRING "Number of new connections on socket queue")
  set(CSP_CONN_RXQUEUE_LEN 16 CACHE STRING "Number of packets in connection queue")
endif()

# Build-time sanity check: CSP v1 and v2 both use 6-bit port fields (max port = 63).
# Outgoing ports occupy the range [CSP_PORT_MAX_BIND+1 .. 63].
math(EXPR CSP_MAX_OUTGOING_PORTS "63 - ${CSP_PORT_MAX_BIND}")
if(CSP_CONN_MAX GREATER CSP_MAX_OUTGOING_PORTS)
  message(FATAL_ERROR "CSP_CONN_MAX (${CSP_CONN_MAX}) exceeds maximum outgoing ports (${CSP_MAX_OUTGOING_PORTS}) for 6-bit port fields")
endif()

set(CSP_RDP_MAX_WINDOW 5 CACHE STRING "Max window size for RDP")
set(CSP_RTABLE_SIZE 10 CACHE STRING "Number of elements in routing table")

set(CSP_USE_RDP ON CACHE BOOL "Reliable Datagram Protocol")
set(CSP_USE_HMAC ON CACHE BOOL "Hash-based message authentication code")
set(CSP_USE_PROMISC ON CACHE BOOL "Promiscious mode")
set(CSP_USE_RTABLE ON CACHE BOOL "Use routing table")
set(CSP_BUFFER_ZERO_CLEAR ON CACHE BOOL "Zero out the packet buffer upon allocation")
set(CSP_FIXUP_V1_ZMQ_LITTLE_ENDIAN ON CACHE BOOL "Use little-endian CSP ID for ZMQ with CSPv1")
option(CSP_TRACEROUTE "Enable traceroute support for simulation" OFF)

message(STATUS "Building for ${CMAKE_SYSTEM_NAME}")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CSP_POSIX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CSP_WINDOWS 1)
    # FM: is this a submodule or old
    target_include_directories(csp_es PUBLIC ${CMAKE_SOURCE_DIR}/peakcan)

    # Create an interface library for PeakCAN includes
    add_library(peakcan_includes INTERFACE)
    target_include_directories(peakcan_includes INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/can/windows/peakcan
    )
    target_link_libraries(csp_es PUBLIC peakcan_includes)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic")
  # Set default CSP_SYSTEM_NAME for embedded builds if not already defined
  if(NOT DEFINED CSP_SYSTEM_NAME)
    set(CSP_SYSTEM_NAME "FreeRTOS")
    message(STATUS "CSP_SYSTEM_NAME not defined, defaulting to FreeRTOS for Generic system")
  endif()

  string(TOLOWER "${CSP_SYSTEM_NAME}" csp_system_name)

  message(STATUS "User specified build for ${csp_system_name}")

  if(csp_system_name STREQUAL "freertos")
    set(CSP_FREERTOS 1)
  elseif(csp_system_name STREQUAL "zephyr")
    set(CSP_ZEPHYR 1)
    message(STATUS "Zephyr build detected")
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

  # Check for endian.h only on Linux to avoid finding host headers during cross-compilation
  include(CheckIncludeFile)
  check_include_file("endian.h" HAVE_ENDIAN_H)
endif()

# Necessary for the examples and sub-directories
set(csp_inc ${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_BINARY_DIR}/include )
set(csp_inc_src  ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Make the .SO file usable by others by providing where to find the headers
# When find_package is used
target_include_directories(csp_es
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include/csp_es>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
set_target_properties(csp_es PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/csp/csp.h")


add_library(csp_common INTERFACE)
if(CSP_POSIX)
  target_compile_options(csp_common INTERFACE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wcast-align -Wpointer-arith -Wwrite-strings -Wno-unused-parameter
    $<$<C_COMPILER_ID:Clang>:-Wno-gnu-zero-variadic-macro-arguments>)
elseif(CSP_WINDOWS)
  target_compile_options(csp_common INTERFACE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wcast-align -Wpointer-arith -Wwrite-strings -Wno-unused-parameter -Wno-int-conversion
    $<$<C_COMPILER_ID:Clang>:-Wno-gnu-zero-variadic-macro-arguments>)
endif()
target_link_libraries(csp_es PRIVATE csp_common)

if(NOT CSP_BUILD_SAMPLES)
  set(CSP_SAMPLES_EXCLUDE "EXCLUDE_FROM_ALL")
endif()


add_subdirectory(src)

# enable_testing() must be called before add_subdirectory(unittests) so that
# add_test() calls inside that directory are registered with CTest.
enable_testing()
add_subdirectory(unittests)

if(CSP_BUILD_SAMPLES)
  add_subdirectory(examples)
  add_subdirectory(samples)
endif()

# Add virtual topology infrastructure (Linux-only, requires CSP_TRACEROUTE=ON and dependencies)
if(CSP_TRACEROUTE AND CSP_POSIX AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Check if required dependencies for virtual topology are available
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_search_module(CJSON libcjson)
    if(CJSON_FOUND)
      message(STATUS "CSP_TRACEROUTE enabled and virtual topology dependencies found, building csp_virtual_topology")
      add_subdirectory(csp_virtual_topology)
    else()
      message(WARNING "libcjson not found - skipping csp_virtual_topology build. Install with: apt-get install libcjson-dev")
    endif()
  else()
    message(WARNING "pkg-config not found - skipping csp_virtual_topology build. Install with: apt-get install pkg-config")
  endif()
else()
  if(NOT CSP_TRACEROUTE AND CSP_POSIX AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "CSP_TRACEROUTE is OFF - skipping csp_virtual_topology build. Use -DCSP_TRACEROUTE=ON to enable")
  endif()
endif()

if(CSP_ENABLE_PYTHON3_BINDINGS)
  find_package(Python3 COMPONENTS Development.Module)
  if(Python3_Development.Module_FOUND)
    Python3_add_library(libcsp_py3 MODULE WITH_SOABI src/bindings/python/pycsp.c)
    target_include_directories(libcsp_py3 PUBLIC ${csp_inc})
    target_link_libraries(libcsp_py3 PUBLIC csp_es)
  else()
    message(FATAL_ERROR "Python development module not found")
  endif()
endif()

configure_file(csp_autoconfig.h.in include/csp/autoconfig.h)

# Make the linux installation separate
if(NOT CSP_ZEPHYR AND CSP_WINDOWS)
	install(TARGETS csp_es LIBRARY COMPONENT runtime)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/;${CMAKE_CURRENT_SOURCE_DIR}/include/;
	  TYPE INCLUDE
	  FILES_MATCHING PATTERN "*.h*"
	)
endif()

# link dependencies
if(CSP_FREERTOS)
    # freertos_kernel target is handled in src/arch/freertos/CMakeLists.txt
    # Only link assertions if the target exists (provided by host project)
    if(TARGET lib_assertions)
        target_link_libraries(csp_es PRIVATE lib_assertions)
    endif()
endif()

# Enable socket on_connect callback feature
if(ENABLE_ON_CONNECT_SOCKET_CALLBACK)
    set(ON_CONNECT_SOCKET_CALLBACK_VALUE 1)
else()
    set(ON_CONNECT_SOCKET_CALLBACK_VALUE 0)
endif()
target_compile_definitions(csp_es PUBLIC ENABLE_ON_CONNECT_SOCKET_CALLBACK=${ON_CONNECT_SOCKET_CALLBACK_VALUE})

# Remove the -pedantic property
get_target_property(CSP_COMPILE_OPTIONS csp_es COMPILE_OPTIONS)
list(REMOVE_ITEM CSP_COMPILE_OPTIONS -Werror)
list(APPEND CSP_COMPILE_OPTIONS -Wno-unused-parameter)
set_target_properties(csp_es PROPERTIES COMPILE_OPTIONS "${CSP_COMPILE_OPTIONS}")

########################################################################
# Packaging and Installation
########################################################################

# Defines the headers for this library.
# https://cmake.org/cmake/help/v3.11/command/target_include_directories.html

# Define the configuration file locations.
if(CSP_POSIX AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  set(PROJECT_CONFIG_VERSION_FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake")
  set(PROJECT_CONFIG_FILE         "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake")

  configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in.csp_es
          ${PROJECT_CONFIG_FILE}
          INSTALL_DESTINATION ${INSTALL_CONFIG_DIR})

  # This auto-generates a version file.
  write_basic_package_version_file(
          ${PROJECT_CONFIG_VERSION_FILE}
          VERSION ${PROJECT_VERSION}
          COMPATIBILITY SameMajorVersion)

  # The INSTALL(TARGET) command specifies where to install specific targets.
  # By specifying EXPORT here, we add these targets to an export group.
  set(INSTALL_TARGETS ${PROJECT_NAME} csp_common)
  if(TARGET csp_yaml)
    list(APPEND INSTALL_TARGETS csp_yaml)
  endif()
  if(TARGET driver_can)
    list(APPEND INSTALL_TARGETS driver_can)
  endif()
  install(TARGETS ${INSTALL_TARGETS}
          EXPORT ${PROJECT_NAME}-targets
          PUBLIC_HEADER DESTINATION ${INSTALL_INCLUDE_DIR}
          RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
          LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
          ARCHIVE DESTINATION ${INSTALL_ARCHIVE_DIR})

  # The INSTALL(FILES) command specifies where to install specific files, such as headers.
  install(FILES ${HEADER_FILES}
          DESTINATION ${INSTALL_INCLUDE_DIR})

  # Always install autoconifg header due to stupid CSP.h dependency
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/csp/autoconfig.h DESTINATION ${INSTALL_INCLUDE_DIR}/csp)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/csp DESTINATION ${INSTALL_INCLUDE_DIR})
  # The INSTALL(FILES) command specifies where to install specific files, such as headers.
  install(FILES
              ${PROJECT_CONFIG_VERSION_FILE}
              ${PROJECT_CONFIG_FILE}
          DESTINATION ${INSTALL_CONFIG_DIR})

  # The INSTALL(EXPORT) command installs a previously defined export group to a script.
  install(EXPORT ${PROJECT_NAME}-targets
          FILE ${PROJECT_NAME}-targets.cmake
          NAMESPACE ${PROJECT_NAME}::
          DESTINATION ${INSTALL_CONFIG_DIR})

  export(EXPORT ${PROJECT_NAME}-targets
         FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake
         NAMESPACE ${PROJECT_NAME}::)

  export(PACKAGE ${PROJECT_NAME})
        set(CPACK_GENERATOR "DEB")
        set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})
        set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
        set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/endurosat/csp-es")
        set(CPACK_PACKAGE_CONTACT "https://www.endurosat.com/contact/")
        set(CPACK_PACKAGE_VENDOR "EnduroSat")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "EnduroSat <https://www.endurosat.com/contact/>")
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "EnduroSat CSP Library (csp_es)"
            CACHE STRING "Package description for the package metadata"
        )
        set(CPACK_VERBATIM_VARIABLES YES)
        set(CPACK_STRIP_FILES YES)
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libyaml (>= 0.2.0),"
                                         "libzmq5 (>= 4.3.3),"
                                         "libjson-c-dev (>= 0.10),"
                                         "libsocketcan-dev (>= 0.0.11),"
        )
        INCLUDE(CPack)
endif()
