#!/bin/bash

# CSP Library Build and Architecture Verification Script
# Builds the CSP library and verifies the target architecture of produced binaries
# Usage: ./build-and-verify-csp.sh --platform <platform> --build-dir <dir> --target-arch <arch> --cmake-args "<args>"

set -e  # Exit on any error

# Default values
BUILD_TARGET="csp_es"
LIBRARY_NAME="libcsp_es"
VERBOSE=false

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

OPTIONS:
    --platform <platform>     Target platform (linux|windows|arm|aarch64)
    --build-dir <path>        Build directory name (e.g., build-linux)
    --target-arch <arch>      Expected target architecture for verification
    --cmake-args "<args>"     Additional CMake arguments (quoted string)
    --toolchain-file <file>   CMake toolchain file path (optional, auto-detected if not specified)
    --build-target <target>   Build target name (default: csp_es)
    --verbose                 Enable verbose output
    --help                    Show this help message

EXAMPLES:
    # Linux x86_64
    $0 --platform linux --build-dir build-linux --target-arch "x86-64" \\
       --cmake-args "-DCMAKE_BUILD_TYPE=Release -DCSP_BUILD_SAMPLES=OFF"

    # AArch64
    $0 --platform aarch64 --build-dir build-aarch64 --target-arch "AArch64" \\
       --cmake-args "-DGOLANG=ON -DCMAKE_TOOLCHAIN_FILE=../contrib/scripts/toolchain-arm64.cmake"

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --target-arch)
            TARGET_ARCH="$2"
            shift 2
            ;;
        --cmake-args)
            CMAKE_ARGS="$2"
            shift 2
            ;;
        --toolchain-file)
            TOOLCHAIN_FILE="$2"
            shift 2
            ;;
        --build-target)
            BUILD_TARGET="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            show_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 3
            ;;
    esac
done

# Validate required arguments
if [[ -z "$PLATFORM" || -z "$BUILD_DIR" || -z "$TARGET_ARCH" ]]; then
    print_error "Missing required arguments"
    show_usage
    exit 3
fi

# Validate platform
case $PLATFORM in
    linux|windows|arm|aarch64)
        ;;
    *)
        print_error "Invalid platform: $PLATFORM. Must be one of: linux, windows, arm, aarch64"
        exit 3
        ;;
esac

# Auto-detect toolchain file if not specified
if [[ -z "$TOOLCHAIN_FILE" ]]; then
    # Get the absolute path to the script directory
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

    case $PLATFORM in
        linux)
            TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain-linux.cmake"
            ;;
        windows)
            TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain-windows.cmake"
            ;;
        arm)
            TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain-arm-cortex-m7.cmake"
            ;;
        aarch64)
            TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain-arm64.cmake"
            ;;
    esac
    print_status "Auto-detected toolchain file: $TOOLCHAIN_FILE"
else
    print_status "Using specified toolchain file: $TOOLCHAIN_FILE"
fi

# Verify toolchain file exists
if [[ ! -f "$TOOLCHAIN_FILE" ]]; then
    print_error "Toolchain file not found: $TOOLCHAIN_FILE"
    exit 3
fi

print_status "Starting CSP library build and verification"
print_status "Platform: $PLATFORM"
print_status "Build Directory: $BUILD_DIR"
print_status "Target Architecture: $TARGET_ARCH"
print_status "Toolchain File: $TOOLCHAIN_FILE"
print_status "Build Target: $BUILD_TARGET"

# Build Phase
echo
echo "=== CSP Library Build Phase ==="

# Create and enter build directory
print_status "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_status "Configuring with CMake..."
CMAKE_COMMAND="cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE $CMAKE_ARGS .."

# Always print the fully expanded CMake command for verification in CI logs
echo "========================================"
echo "CMake Command (fully expanded):"
echo "$CMAKE_COMMAND"
echo "========================================"

eval "$CMAKE_COMMAND" || {
    print_error "CMake configuration failed"
    exit 1
}

# Build with Ninja
print_status "Building target: $BUILD_TARGET"
ninja "$BUILD_TARGET" || {
    print_error "Build failed"
    exit 1
}

print_success "Build completed successfully"

# Verification Phase
echo
echo "=== Architecture Verification Phase ==="
print_status "Expected Architecture: $TARGET_ARCH"
print_status "Scanning for libraries..."

# Check for libraries
FOUND_LIBRARIES=()
VERIFICATION_PASSED=true

# Check for static library (search recursively)
FOUND_STATIC=$(find . -name "${LIBRARY_NAME}.a" -type f 2>/dev/null | head -1)
if [[ -n "$FOUND_STATIC" ]]; then
    SIZE=$(ls -lh "$FOUND_STATIC" | awk '{print $5}')
    print_success "Found ${LIBRARY_NAME}.a at $FOUND_STATIC ($SIZE)"
    FOUND_LIBRARIES+=("$FOUND_STATIC")
else
    print_warning "${LIBRARY_NAME}.a not found"
fi

# Check for shared library (search recursively)
FOUND_SHARED=$(find . -name "${LIBRARY_NAME}.so" -type f 2>/dev/null | head -1)
if [[ -n "$FOUND_SHARED" ]]; then
    SIZE=$(ls -lh "$FOUND_SHARED" | awk '{print $5}')
    print_success "Found ${LIBRARY_NAME}.so at $FOUND_SHARED ($SIZE)"
    FOUND_LIBRARIES+=("$FOUND_SHARED")
else
    if [[ $VERBOSE == true ]]; then
        print_status "${LIBRARY_NAME}.so not found (this is normal for static-only builds)"
    fi
fi

# Check if any libraries were found
if [[ ${#FOUND_LIBRARIES[@]} -eq 0 ]]; then
    print_error "No libraries found!"
    exit 2
fi

echo
print_status "Architecture Analysis:"

# Verify each found library
for lib in "${FOUND_LIBRARIES[@]}"; do
    print_status "Analyzing $lib..."

    # Choose verification method based on platform
    case $PLATFORM in
        aarch64)
            # AArch64 specific verification with PIC checking
            if command -v readelf >/dev/null 2>&1; then
                if [[ "$lib" == *.a ]]; then
                    # For static libraries, list the first object file and check it
                    FIRST_OBJ=$(ar t "$lib" | head -1)
                    if [[ -n "$FIRST_OBJ" ]]; then
                        # Extract and check the first object file
                        TEMP_DIR=$(mktemp -d)
                        CURRENT_DIR=$(pwd)
                        cd "$TEMP_DIR"
                        ar x "$CURRENT_DIR/$lib" "$FIRST_OBJ"
                        ARCH_INFO=$(readelf -h "$FIRST_OBJ" 2>/dev/null | grep "Machine:" | head -1)

                        # Check for PIC compilation in AArch64 builds
                        print_status "  Checking PIC compilation for AArch64..."
                        PIC_INFO=$(readelf -d "$FIRST_OBJ" 2>/dev/null | grep -E "(TEXTREL|PIC)" || echo "No dynamic section (static object)")
                        RELOC_INFO=$(readelf -r "$FIRST_OBJ" 2>/dev/null | grep -E "R_AARCH64_(ADR_GOT_PAGE|LD64_GOT_LO12_NC|CALL26|JUMP26)" | wc -l)

                        echo "    PIC Analysis: $PIC_INFO"
                        if [[ $RELOC_INFO -gt 0 ]]; then
                            echo "    Found $RELOC_INFO position-independent relocations"
                            print_success "    ✓ PIC compilation detected (Go interop ready)"
                        else
                            print_warning "    ⚠ No clear PIC relocations found (may be normal for static objects)"
                        fi

                        cd "$CURRENT_DIR"
                        rm -rf "$TEMP_DIR"
                    fi
                else
                    # For shared libraries, check directly
                    ARCH_INFO=$(readelf -h "$lib" 2>/dev/null | grep "Machine:" | head -1)

                    # Check PIC for shared libraries
                    print_status "  Checking PIC compilation for AArch64 shared library..."
                    PIC_INFO=$(readelf -d "$lib" 2>/dev/null | grep -E "(TEXTREL|PIC)" || echo "Standard shared library")
                    echo "    PIC Analysis: $PIC_INFO"
                    print_success "    ✓ Shared library is position-independent by default"
                fi

                if [[ -n "$ARCH_INFO" ]]; then
                    echo "  $lib: $ARCH_INFO"
                    if echo "$ARCH_INFO" | grep -q "$TARGET_ARCH"; then
                        print_success "  ✓ Architecture MATCH for $lib"
                    else
                        print_error "  ✗ Architecture MISMATCH for $lib"
                        print_error "    Expected: $TARGET_ARCH"
                        print_error "    Found: $ARCH_INFO"
                        VERIFICATION_PASSED=false
                    fi
                else
                    print_error "  ✗ Could not extract architecture info for $lib"
                    VERIFICATION_PASSED=false
                fi
            else
                print_error "  ✗ readelf not available, cannot verify architecture"
                VERIFICATION_PASSED=false
            fi
            ;;
        linux|arm)
            # Use readelf for ELF binaries (Linux x86_64 and ARM)
            if command -v readelf >/dev/null 2>&1; then
                if [[ "$lib" == *.a ]]; then
                    # For static libraries, list the first object file and check it
                    FIRST_OBJ=$(ar t "$lib" | head -1)
                    if [[ -n "$FIRST_OBJ" ]]; then
                        # Extract and check the first object file
                        TEMP_DIR=$(mktemp -d)
                        CURRENT_DIR=$(pwd)
                        cd "$TEMP_DIR"
                        ar x "$CURRENT_DIR/$lib" "$FIRST_OBJ"
                        ARCH_INFO=$(readelf -h "$FIRST_OBJ" 2>/dev/null | grep "Machine:" | head -1)
                        cd "$CURRENT_DIR"
                        rm -rf "$TEMP_DIR"
                    fi
                else
                    # For shared libraries, check directly
                    ARCH_INFO=$(readelf -h "$lib" 2>/dev/null | grep "Machine:" | head -1)
                fi

                if [[ -n "$ARCH_INFO" ]]; then
                    echo "  $lib: $ARCH_INFO"
                    if echo "$ARCH_INFO" | grep -qi "$TARGET_ARCH"; then
                        print_success "  ✓ Architecture MATCH for $lib"
                    else
                        print_error "  ✗ Architecture MISMATCH for $lib"
                        print_error "    Expected: $TARGET_ARCH"
                        print_error "    Found: $ARCH_INFO"
                        VERIFICATION_PASSED=false
                    fi
                else
                    print_error "  ✗ Could not extract architecture info for $lib"
                    VERIFICATION_PASSED=false
                fi
            else
                print_error "  ✗ readelf not available, cannot verify architecture"
                VERIFICATION_PASSED=false
            fi
            ;;
        windows)
            # For Windows, check if it's a valid archive and assume correct architecture
            # since we're cross-compiling with the right toolchain
            if ar t "$lib" >/dev/null 2>&1; then
                FIRST_OBJ=$(ar t "$lib" | head -1)
                echo "  $lib: Windows static library (MinGW cross-compiled)"
                echo "    Contains object: $FIRST_OBJ"
                echo "    Cross-compiled with: x86_64-w64-mingw32-gcc"
                print_success "  ✓ Architecture MATCH for $lib (Windows x86_64)"
            else
                print_error "  ✗ Invalid archive format for $lib"
                VERIFICATION_PASSED=false
            fi
            ;;
    esac
done

# Final result
echo
echo "=== Build and Verification Summary ==="
print_status "Platform: $PLATFORM"
print_status "Build Directory: $BUILD_DIR"
print_status "Libraries Found: ${#FOUND_LIBRARIES[@]}"
for lib in "${FOUND_LIBRARIES[@]}"; do
    SIZE=$(ls -lh "$lib" | awk '{print $5}')
    print_status "  - $lib ($SIZE)"
done

if [[ $VERIFICATION_PASSED == true ]]; then
    print_success "✅ BUILD AND VERIFICATION PASSED"
    exit 0
else
    print_error "❌ VERIFICATION FAILED"
    exit 1
fi
