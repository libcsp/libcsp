cmake_minimum_required(VERSION 3.20)

# Find required dependencies
# Note: PkgConfig and CJSON should already be found by parent CMakeLists.txt
# but we verify here for standalone builds
find_package(PkgConfig QUIET)
if(NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "pkg-config not found. Install with: apt-get install pkg-config")
endif()

find_package(Threads REQUIRED)

# csp_es is built in the parent project, so it should be available
set(CSP_ES_AVAILABLE TRUE)
message(STATUS "CSP_ES is available from parent project")

# Find cJSON library
pkg_search_module(CJSON libcjson)
if(CJSON_FOUND)
    message(STATUS "Found cJSON: ${CJSON_VERSION}")
else()
    message(FATAL_ERROR "cJSON library not found. Install with: apt-get install libcjson-dev")
endif()

# Find ZMQ library
pkg_search_module(LIBZMQ libzmq)
if(LIBZMQ_FOUND)
    message(STATUS "Found libzmq: ${LIBZMQ_VERSION}")
else()
    message(WARNING "libzmq not found via pkg-config")
endif()

# Source files for config parser library
set(CONFIG_PARSER_SOURCES
    src/config_parser.c
)

set(CONFIG_PARSER_HEADERS
    include/config_parser.h
)

# Create config parser library
add_library(config_parser STATIC
    ${CONFIG_PARSER_SOURCES}
    ${CONFIG_PARSER_HEADERS}
)

target_link_libraries(config_parser
    ${CJSON_LIBRARIES}
)

# Add csp_es if available
if(CSP_ES_AVAILABLE)
    target_link_libraries(config_parser csp_es::csp_es)
endif()

target_include_directories(config_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CJSON_INCLUDE_DIRS}
)

# Virtual node executable
if(CSP_ES_AVAILABLE)
    add_executable(csp_virtual_node src/csp_virtual_node.c)

    target_link_libraries(csp_virtual_node
        config_parser
        ${CJSON_LIBRARIES}
        Threads::Threads
    )

    # Add ZMQ library if found
    if(LIBZMQ_FOUND)
        target_include_directories(csp_virtual_node PRIVATE ${LIBZMQ_INCLUDE_DIRS})
        target_link_libraries(csp_virtual_node ${LIBZMQ_LIBRARIES})
    else()
        # Try to find zmq library directly
        find_library(ZMQ_LIBRARY NAMES zmq)
        if(ZMQ_LIBRARY)
            message(STATUS "Found libzmq library: ${ZMQ_LIBRARY}")
            target_link_libraries(csp_virtual_node ${ZMQ_LIBRARY})
        else()
            message(WARNING "libzmq not found, control interface may not work")
        endif()
    endif()

    # Add CSP_ES_AVAILABLE define
    target_compile_definitions(csp_virtual_node PRIVATE CSP_ES_AVAILABLE)
    target_link_libraries(csp_virtual_node csp_es::csp_es)

    # ZMQ Proxy executable (renamed to avoid conflict with examples/zmqproxy)
    add_executable(csp_virtual_zmqproxy src/zmqproxy.c)

    target_link_libraries(csp_virtual_zmqproxy
        csp_es
        csp_common
        Threads::Threads
    )

    if(LIBZMQ_FOUND)
        target_include_directories(csp_virtual_zmqproxy PRIVATE ${LIBZMQ_INCLUDE_DIRS})
        target_link_libraries(csp_virtual_zmqproxy ${LIBZMQ_LIBRARIES})
    else()
        find_library(ZMQ_LIBRARY NAMES zmq)
        if(ZMQ_LIBRARY)
            target_link_libraries(csp_virtual_zmqproxy ${ZMQ_LIBRARY})
        endif()
    endif()
endif()

# Install targets
install(TARGETS config_parser
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${CONFIG_PARSER_HEADERS}
    DESTINATION include/csp_virtual_topology
)

if(CSP_ES_AVAILABLE)
    install(TARGETS csp_virtual_node csp_virtual_zmqproxy
        RUNTIME DESTINATION bin
    )
endif()

