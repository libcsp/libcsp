cmake_minimum_required(VERSION 3.20)

project(csp_virtual_topology VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add virtual node infrastructure subdirectory
add_subdirectory(virtual_node_infrastructure)

# Define package directory in build folder
set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/csp_virtual_topology_package")

# Create custom target to build the complete package
# This target depends on the executables being built first
add_custom_target(csp_virtual_topology_package ALL
    COMMENT "Building CSP Virtual Topology complete package"
    DEPENDS csp_virtual_node csp_virtual_zmqproxy
)

# Create package directory structure
add_custom_command(TARGET csp_virtual_topology_package PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}/bin"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}/topologies"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}/web/static"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}/web/templates"
    COMMENT "Creating package directory structure"
)

# Copy executables after they are built
add_custom_command(TARGET csp_virtual_topology_package POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/virtual_node_infrastructure/csp_virtual_node"
        "${PACKAGE_DIR}/bin/csp_virtual_node"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/virtual_node_infrastructure/csp_virtual_zmqproxy"
        "${PACKAGE_DIR}/bin/csp_virtual_zmqproxy"
    COMMENT "Copying executables to package"
)

# Copy Python scripts
add_custom_command(TARGET csp_virtual_topology_package POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/topology_launcher.py"
        "${PACKAGE_DIR}/bin/csp_topology_launcher"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/trace_collector.py"
        "${PACKAGE_DIR}/bin/trace_collector.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_trace_log.py"
        "${PACKAGE_DIR}/bin/analyze_trace_log.py"
    COMMENT "Copying Python scripts to package"
)

# Copy topology examples
add_custom_command(TARGET csp_virtual_topology_package POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/topologies"
        "${PACKAGE_DIR}/topologies"
    COMMENT "Copying topology examples to package"
)

# Copy web assets
add_custom_command(TARGET csp_virtual_topology_package POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/web/static"
        "${PACKAGE_DIR}/web/static"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/web/templates"
        "${PACKAGE_DIR}/web/templates"
    COMMENT "Copying web assets to package"
)

# Copy requirements file
add_custom_command(TARGET csp_virtual_topology_package POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt"
        "${PACKAGE_DIR}/requirements.txt"
    COMMENT "Copying requirements.txt to package"
)

# Install topology examples (for traditional install)
install(DIRECTORY topologies/
    DESTINATION share/csp_virtual_topology/topologies
    FILES_MATCHING PATTERN "*.json"
)

# Install tools (for traditional install)
install(PROGRAMS tools/topology_launcher.py
    DESTINATION bin
    RENAME csp_topology_launcher
)

# Add a final message target to display package information after build
add_custom_command(TARGET csp_virtual_topology_package POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "CSP Virtual Topology Package Created"
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Package Location: ${PACKAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Package Contents:"
    COMMAND ${CMAKE_COMMAND} -E echo "  bin/ - executables and Python scripts"
    COMMAND ${CMAKE_COMMAND} -E echo "  topologies/ - JSON configuration files"
    COMMAND ${CMAKE_COMMAND} -E echo "  web/ - Web UI assets"
    COMMAND ${CMAKE_COMMAND} -E echo "  requirements.txt - Python dependencies"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Ready for CI artifacts"
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
)

# Print configuration summary
message(STATUS "")
message(STATUS "CSP Virtual Topology Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Virtual Node Infrastructure: Included")
message(STATUS "  Package Directory: ${PACKAGE_DIR}")
message(STATUS "")

