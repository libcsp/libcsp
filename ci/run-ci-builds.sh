#!/usr/bin/env bash
# run-ci-builds.sh — Local CI build runner for csp_es
#
# Replicates GitHub Actions CI jobs inside the csp-es-ci Docker container.
# Use this as a pre-push smoke test before relying on GitHub Actions.
#
# Usage (from repo root):
#   docker run --rm -v $(pwd):/workspace -w /workspace --user $(id -u):$(id -g) csp-es-ci \
#     bash ci/run-ci-builds.sh [--skip-tests]
#
# Options:
#   --skip-tests   Skip functional/runtime tests (builds + docs still run)

set -e

# --- Argument parsing ---
SKIP_TESTS=false
for arg in "$@"; do
    case $arg in
        --skip-tests) SKIP_TESTS=true ;;
        --help)
            grep '^#' "$0" | head -20 | sed 's/^# \?//'
            exit 0
            ;;
        *)
            echo "Unknown argument: $arg"
            exit 1
            ;;
    esac
done

# --- Colors ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

print_header() { echo -e "\n${CYAN}${BOLD}=== $1 ===${NC}"; }
print_status() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[PASS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
print_error()   { echo -e "${RED}[FAIL]${NC} $1"; }

# --- Job tracking ---
JOBS_PASSED=()
JOBS_FAILED=()

run_job() {
    local name="$1"
    local func="$2"
    print_header "$name"
    if $func; then
        JOBS_PASSED+=("$name")
        print_success "$name PASSED"
    else
        JOBS_FAILED+=("$name")
        print_error "$name FAILED"
        # Continue running remaining jobs even on failure
    fi
}

# --- Common CMake args (mirrors build-endurosat.yml COMMON_CMAKE_ARGS) ---
COMMON_CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE=Release
    -DCSP_BUILD_SAMPLES=OFF
    -DCSP_USE_RDP=ON
    -DCSP_USE_HMAC=ON
    -DCSP_USE_PROMISC=ON
    -DCSP_USE_RTABLE=ON
    -DCSP_ENABLE_PYTHON3_BINDINGS=OFF
    -DCSP_ENABLE_CSP_PRINT=OFF
    -DCSP_HAVE_STDIO=OFF
    -DCSP_PRINT_STDIO=OFF
)

VERIFY_SCRIPT="contrib/scripts/build-and-verify-csp.sh"

# ============================================================
# EnduroSat builds (mirrors build-endurosat.yml)
# ============================================================

job_endurosat_linux() {
    rm -rf build-linux
    bash "$VERIFY_SCRIPT" \
        --platform linux \
        --build-dir build-linux \
        --target-arch "x86-64" \
        --cmake-args "${COMMON_CMAKE_ARGS[*]}"
}

job_endurosat_windows() {
    rm -rf build-windows
    bash "$VERIFY_SCRIPT" \
        --platform windows \
        --build-dir build-windows \
        --target-arch "x86-64" \
        --cmake-args "${COMMON_CMAKE_ARGS[*]}"
}

job_endurosat_arm() {
    rm -rf build-arm
    bash "$VERIFY_SCRIPT" \
        --platform arm \
        --build-dir build-arm \
        --target-arch "ARM" \
        --cmake-args "-DCSP_SYSTEM_NAME=FreeRTOS ${COMMON_CMAKE_ARGS[*]}"
}

job_endurosat_linux_go() {
    rm -rf build-linux-go
    bash "$VERIFY_SCRIPT" \
        --platform linux \
        --build-dir build-linux-go \
        --target-arch "x86-64" \
        --cmake-args "-DGOLANG=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON ${COMMON_CMAKE_ARGS[*]}"
}

# ============================================================
# libcsp upstream builds (mirrors build-test.yml)
# ============================================================

job_upstream_cmake() {
    rm -rf build
    python3 examples/buildall.py --build-system=cmake
}

job_upstream_meson() {
    rm -rf build
    python3 examples/buildall.py --build-system=meson
}

job_upstream_waf() {
    rm -rf build
    python3 examples/buildall.py --build-system=waf
}

# ============================================================
# Software-only functional tests (mirrors build-test.yml test steps)
# Requires: cmake build artifacts in ./build/examples/
# CAN/vcan0 tests are excluded (hardware-dependent, CI-only)
# ============================================================

job_functional_tests() {
    # Ensure cmake build is available
    if [[ ! -f build/examples/csp_server_client ]]; then
        print_status "Rebuilding cmake artifacts for functional tests..."
        rm -rf build
        python3 examples/buildall.py --build-system=cmake
    fi

    print_status "Running loopback tests..."
    ./build/examples/csp_arch
    ./build/examples/csp_server_client -T 10

    print_status "Running KISS client test (CSP v1)..."
    socat -d -d -d pty,raw,echo=0,link=/tmp/pty1 pty,raw,echo=0,link=/tmp/pty2 &
    SOCAT_PID=$!
    sleep 1
    ./build/examples/csp_server -k /tmp/pty1 -a 1 -T 10 -v 1 &
    ./build/examples/csp_client -k /tmp/pty2 -a 2 -C 1 -t -v 1
    kill $SOCAT_PID 2>/dev/null || true; wait $SOCAT_PID 2>/dev/null || true

    print_status "Running KISS server test (CSP v1)..."
    socat -d -d -d pty,raw,echo=0,link=/tmp/pty1 pty,raw,echo=0,link=/tmp/pty2 &
    SOCAT_PID=$!
    sleep 1
    ./build/examples/csp_client -k /tmp/pty2 -a 2 -C 1 -T 10 -v 1 &
    ./build/examples/csp_server -k /tmp/pty1 -a 1 -t -v 1
    kill $SOCAT_PID 2>/dev/null || true; wait $SOCAT_PID 2>/dev/null || true

    print_status "Running ZMQ client test (CSP v1)..."
    ./build/examples/zmqproxy &
    ZMQPROXY_PID=$!
    ./build/examples/csp_server -z localhost -a 1 -T 10 -v 1 &
    ./build/examples/csp_client -z localhost -a 2 -C 1 -t -v 1
    kill $ZMQPROXY_PID 2>/dev/null || true; wait $ZMQPROXY_PID 2>/dev/null || true

    print_status "Running ZMQ server test (CSP v1)..."
    ./build/examples/zmqproxy &
    ZMQPROXY_PID=$!
    ./build/examples/csp_client -z localhost -a 2 -C 1 -T 10 -v 1 &
    ./build/examples/csp_server -z localhost -a 1 -t -v 1
    kill $ZMQPROXY_PID 2>/dev/null || true; wait $ZMQPROXY_PID 2>/dev/null || true
}

# ============================================================
# Python bindings — build only (mirrors build-test-python.yml build steps)
# Tests are in job_python_binding_tests below
# ============================================================

job_python_bindings_cmake() {
    rm -rf builddir

    print_status "Building Python bindings (cmake)..."
    cmake -GNinja -B builddir \
        -DCSP_ENABLE_PYTHON3_BINDINGS=1 \
        -DCSP_USE_RTABLE=1 \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    ninja -C builddir
}

job_python_bindings_meson() {
    rm -rf builddir-meson

    print_status "Building Python bindings (meson)..."
    meson setup builddir-meson \
        -Denable_python3_bindings=true \
        -Duse_rtable=true
    ninja -C builddir-meson
}

job_python_binding_tests() {
    # Ensure cmake bindings are built (tests use cmake builddir)
    if [[ ! -f builddir/libcsp_py3*.so ]]; then
        print_status "Rebuilding cmake Python bindings for tests..."
        job_python_bindings_cmake
    fi

    # Ensure cmake examples are built for zmqproxy
    if [[ ! -f build/examples/zmqproxy ]]; then
        rm -rf build
        python3 examples/buildall.py --build-system=cmake
    fi

    print_status "Running ZMQ Python binding test..."
    build/examples/zmqproxy &
    ZMQPROXY_PID=$!
    PYTHONPATH=builddir python3 examples/python_bindings_example_server.py \
        -z localhost -a 3 &
    PYTHONPATH=builddir python3 examples/python_bindings_example_client.py \
        -z localhost -s 3 -a 2
    kill $ZMQPROXY_PID 2>/dev/null || true; wait $ZMQPROXY_PID 2>/dev/null || true

    print_status "Running KISS Python binding test..."
    socat -d -d -d pty,raw,echo=0,link=/tmp/pty1 pty,raw,echo=0,link=/tmp/pty2 &
    SOCAT_PID=$!
    sleep 1
    PYTHONPATH=builddir python3 examples/python_bindings_example_server.py \
        -k /tmp/pty2 -a 1 &
    PYTHONPATH=builddir python3 examples/python_bindings_example_client.py \
        -k /tmp/pty1 -a 2 -s 1
    kill $SOCAT_PID 2>/dev/null || true; wait $SOCAT_PID 2>/dev/null || true
}

# ============================================================
# Sphinx docs build (mirrors develop-build-sphinx-docs.yml)
# Requires: doc/requirements.txt, libclang-dev, cmake
# ============================================================

job_sphinx_docs() {
    rm -rf build-docs

    print_status "Building Sphinx documentation..."
    cmake -B build-docs -S doc
    cmake --build build-docs
}

# ============================================================
# ABI tooling check (partial mirror of abi-checker.yml)
# Verifies ABI toolchain works; full 2-branch comparison needs git history
# ============================================================

job_abi_tooling() {
    rm -rf build-abi /tmp/abi-dumps
    mkdir -p /tmp/abi-dumps

    print_status "Building debug library for ABI dump..."
    cmake -GNinja -B build-abi \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCSP_USE_RTABLE=1 \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_SHARED_LIBS=ON
    ninja -C build-abi

    # Find the shared library (may be versioned, e.g. libcsp_es.so.1.0.0)
    LIB=$(find build-abi -name "libcsp*.so*" -type f | head -1)
    if [[ -z "$LIB" ]]; then
        print_error "No shared library found in build-abi — cannot produce ABI dump"
        return 1
    fi

    print_status "Producing ABI dump from $LIB..."
    abi-dumper "$LIB" -lver "local" -o /tmp/abi-dumps/libcsp-local.dump
    print_success "ABI dump produced: /tmp/abi-dumps/libcsp-local.dump"
}

# ============================================================
# Main
# ============================================================

echo -e "${BOLD}csp_es local CI build runner${NC}"
echo "Working directory: $(pwd)"
echo "Skip tests: $SKIP_TESTS"
echo "Date: $(date)"

# EnduroSat builds
run_job "EnduroSat: Linux x86_64"         job_endurosat_linux
run_job "EnduroSat: Windows x86_64 (MinGW)" job_endurosat_windows
run_job "EnduroSat: ARM Cortex-M7"        job_endurosat_arm
run_job "EnduroSat: Linux x86_64 + Go"    job_endurosat_linux_go

# Upstream libcsp builds
run_job "libcsp: cmake build"             job_upstream_cmake
run_job "libcsp: meson build"             job_upstream_meson
run_job "libcsp: waf build"               job_upstream_waf

# Python bindings builds (always run)
run_job "Python bindings: cmake build"    job_python_bindings_cmake
run_job "Python bindings: meson build"    job_python_bindings_meson

# Sphinx docs build (always run)
run_job "Sphinx docs build"               job_sphinx_docs

# Functional tests (software-only)
if [[ "$SKIP_TESTS" == false ]]; then
    run_job "Functional tests (loopback/KISS/ZMQ)" job_functional_tests
    run_job "Python binding tests (ZMQ/KISS)"      job_python_binding_tests
else
    print_warning "Skipping functional tests (--skip-tests)"
fi

# ABI tooling
run_job "ABI tooling check"               job_abi_tooling

# --- Summary ---
print_header "CI BUILD SUMMARY"
TOTAL=$(( ${#JOBS_PASSED[@]} + ${#JOBS_FAILED[@]} ))
echo -e "Total: $TOTAL  |  ${GREEN}Passed: ${#JOBS_PASSED[@]}${NC}  |  ${RED}Failed: ${#JOBS_FAILED[@]}${NC}"
echo

if [[ ${#JOBS_PASSED[@]} -gt 0 ]]; then
    echo -e "${GREEN}PASSED:${NC}"
    for j in "${JOBS_PASSED[@]}"; do echo "  ✓ $j"; done
fi

if [[ ${#JOBS_FAILED[@]} -gt 0 ]]; then
    echo -e "\n${RED}FAILED:${NC}"
    for j in "${JOBS_FAILED[@]}"; do echo "  ✗ $j"; done
    echo
    print_error "Some jobs failed. Review output above."
    exit 1
fi

echo
print_success "All CI jobs passed."
