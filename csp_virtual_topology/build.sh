#!/bin/bash

################################################################################
# CSP Virtual Node Infrastructure Build Script
#
# This script builds the complete virtual node infrastructure including:
# - config_parser library
# - csp_virtual_node executable
# - zmqproxy executable
#
# Usage:
#   ./build.sh [OPTIONS]
#
# Options:
#   -h, --help              Show this help message
#   -c, --clean             Clean build directory before building
#   -r, --release           Build in Release mode (default: Debug)
#   -j, --jobs N            Number of parallel build jobs (default: 4)
#   -i, --install           Install after building
#   -p, --prefix PATH       Installation prefix (default: /usr/local)
#   -v, --verbose           Verbose build output
#   --no-color              Disable colored output
#
# Examples:
#   ./build.sh                          # Build in Debug mode
#   ./build.sh --clean --release        # Clean build in Release mode
#   ./build.sh --install --prefix ~/.local  # Build and install locally
#
################################################################################

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_DIR="build"
BUILD_TYPE="Debug"
JOBS=4
INSTALL=false
INSTALL_PREFIX="/usr/local"
VERBOSE=false
USE_COLOR=true
CLEAN=false

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

################################################################################
# Helper Functions
################################################################################

print_header() {
    if [ "$USE_COLOR" = true ]; then
        echo -e "${BLUE}==== $1 ====${NC}"
    else
        echo "==== $1 ===="
    fi
}

print_success() {
    if [ "$USE_COLOR" = true ]; then
        echo -e "${GREEN}✓ $1${NC}"
    else
        echo "✓ $1"
    fi
}

print_error() {
    if [ "$USE_COLOR" = true ]; then
        echo -e "${RED}✗ $1${NC}" >&2
    else
        echo "✗ $1" >&2
    fi
}

print_warning() {
    if [ "$USE_COLOR" = true ]; then
        echo -e "${YELLOW}⚠ $1${NC}"
    else
        echo "⚠ $1"
    fi
}

print_info() {
    if [ "$USE_COLOR" = true ]; then
        echo -e "${BLUE}ℹ $1${NC}"
    else
        echo "ℹ $1"
    fi
}

show_help() {
    head -n 30 "$0" | tail -n +3
}

check_dependencies() {
    print_header "Checking Dependencies"

    local missing=false

    # Check for cmake
    if ! command -v cmake &> /dev/null; then
        print_error "cmake not found. Install with: sudo apt-get install cmake"
        missing=true
    else
        print_success "cmake found ($(cmake --version | head -n1))"
    fi

    # Check for ninja or make
    if command -v ninja &> /dev/null; then
        print_success "ninja found"
        BUILD_TOOL="ninja"
    elif command -v make &> /dev/null; then
        print_success "make found"
        BUILD_TOOL="make"
    else
        print_error "Neither ninja nor make found"
        missing=true
    fi

    # Check for pkg-config
    if ! command -v pkg-config &> /dev/null; then
        print_error "pkg-config not found. Install with: sudo apt-get install pkg-config"
        missing=true
    else
        print_success "pkg-config found"
    fi

    # Check for required libraries
    if ! pkg-config --exists libcjson; then
        print_error "libcjson not found. Install with: sudo apt-get install libcjson-dev"
        missing=true
    else
        print_success "libcjson found ($(pkg-config --modversion libcjson))"
    fi

    if ! pkg-config --exists libzmq; then
        print_warning "libzmq not found. Install with: sudo apt-get install libzmq3-dev"
    else
        print_success "libzmq found ($(pkg-config --modversion libzmq))"
    fi

    if [ "$missing" = true ]; then
        print_error "Missing required dependencies"
        return 1
    fi

    return 0
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                CLEAN=true
                shift
                ;;
            -r|--release)
                BUILD_TYPE="Release"
                shift
                ;;
            -j|--jobs)
                JOBS="$2"
                shift 2
                ;;
            -i|--install)
                INSTALL=true
                shift
                ;;
            -p|--prefix)
                INSTALL_PREFIX="$2"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            --no-color)
                USE_COLOR=false
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

clean_build() {
    print_header "Cleaning Build Directory"
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    else
        print_info "Build directory does not exist"
    fi
}

configure_build() {
    print_header "Configuring Build"

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
        "-DCSP_TRACEROUTE=ON"
    )

    if [ "$BUILD_TOOL" = "ninja" ]; then
        cmake_args+=("-GNinja")
    fi

    if [ "$VERBOSE" = true ]; then
        cmake_args+=("--debug-output")
    fi

    print_info "CMake arguments: ${cmake_args[*]}"

    # Check if we're in the main CSP project or standalone
    if [ -f "../../CMakeLists.txt" ] && grep -q "add_subdirectory(csp_virtual_topology)" "../../CMakeLists.txt" 2>/dev/null; then
        print_info "Building as part of main CSP project"
        cmake "${cmake_args[@]}" ../..
    else
        print_warning "Building standalone (csp_es must be installed)"
        cmake "${cmake_args[@]}" ..
    fi

    cd - > /dev/null
    print_success "Build configured"
}

build_project() {
    print_header "Building Virtual Node Infrastructure"

    cd "$BUILD_DIR"

    if [ "$BUILD_TOOL" = "ninja" ]; then
        if [ "$VERBOSE" = true ]; then
            ninja -j "$JOBS"
        else
            ninja -j "$JOBS" 2>&1 | tail -20
        fi
    else
        if [ "$VERBOSE" = true ]; then
            make -j "$JOBS"
        else
            make -j "$JOBS" 2>&1 | tail -20
        fi
    fi

    cd - > /dev/null
    print_success "Build completed"
}

install_project() {
    print_header "Installing Virtual Node Infrastructure"

    cd "$BUILD_DIR"

    if [ "$BUILD_TOOL" = "ninja" ]; then
        ninja install
    else
        make install
    fi

    cd - > /dev/null
    print_success "Installation completed to $INSTALL_PREFIX"
}

show_build_summary() {
    print_header "Build Summary"

    print_info "Build Type: $BUILD_TYPE"
    print_info "Build Directory: $BUILD_DIR"
    print_info "Build Tool: $BUILD_TOOL"
    print_info "Parallel Jobs: $JOBS"

    if [ "$INSTALL" = true ]; then
        print_info "Installation Prefix: $INSTALL_PREFIX"
    fi

    echo ""
    print_info "Binaries:"
    if [ -f "$BUILD_DIR/csp_virtual_node" ]; then
        print_success "csp_virtual_node: $BUILD_DIR/csp_virtual_node"
    fi
    if [ -f "$BUILD_DIR/zmqproxy" ]; then
        print_success "zmqproxy: $BUILD_DIR/zmqproxy"
    fi
    if [ -f "$BUILD_DIR/libconfig_parser.a" ]; then
        print_success "libconfig_parser.a: $BUILD_DIR/libconfig_parser.a"
    fi
}

################################################################################
# Main Script
################################################################################

main() {
    parse_arguments "$@"

    print_header "CSP Virtual Node Infrastructure Build"
    print_info "Script directory: $SCRIPT_DIR"

    # Check dependencies
    if ! check_dependencies; then
        exit 1
    fi

    # Clean if requested
    if [ "$CLEAN" = true ]; then
        clean_build
    fi

    # Configure and build
    configure_build
    build_project

    # Install if requested
    if [ "$INSTALL" = true ]; then
        install_project
    fi

    # Show summary
    show_build_summary

    print_success "Build process completed successfully!"
}

# Run main function
main "$@"

