# csp_es — EnduroSat fork of libcsp

`csp_es` is EnduroSat's maintained fork of [libcsp](https://github.com/libcsp/libcsp),
the Cubesat Space Protocol library. It stays close to the upstream implementation while
adding targeted enhancements:

- Improved portability across diverse hardware and software platforms.
- Correct operation on 16-bit MCUs.
- Improved target system detection with CMake builds.
- Go interoperability (CGO/Position Independent Code support for AArch64).
- Multi-architecture build scripts and toolchain files for Linux, Windows, ARM, and AArch64.
- CSP Virtual Topology Simulator for network testing without physical hardware.


# Building CSP for Different Architectures

This project includes a unified build and verification script that makes it easy to build the CSP library for different target architectures. Whether you're developing locally or setting up CI/CD pipelines, the `build-and-verify-csp.sh` script provides a consistent and reliable way to build and verify your CSP library.

## Quick Start

The easiest way to build CSP is using the unified build script:

```bash
# Build for your native Linux system
./contrib/scripts/build-and-verify-csp.sh --platform linux --build-dir build-linux --target-arch "x86-64" --cmake-args "-DCMAKE_BUILD_TYPE=Release"

# Build for Windows (cross-compilation)
./contrib/scripts/build-and-verify-csp.sh --platform windows --build-dir build-windows --target-arch "PE32+" --cmake-args "-DCMAKE_BUILD_TYPE=Release"

# Build for ARM Cortex-M7 (embedded/FreeRTOS)
export ARM_ARCH=cortex-m7
./contrib/scripts/build-and-verify-csp.sh --platform arm --build-dir build-arm --target-arch "ARM" --cmake-args "-DCMAKE_BUILD_TYPE=Release -DCSP_SYSTEM_NAME=FreeRTOS"

# Build for AArch64 with Go interoperability
./contrib/scripts/build-and-verify-csp.sh --platform aarch64 --build-dir build-aarch64 --target-arch "aarch64" --cmake-args "-DGOLANG=ON -DCMAKE_BUILD_TYPE=Release"
```

## Local CI / Pre-push Smoke Test

A Docker image (`ci/Dockerfile`) replicates the full GitHub Actions CI environment locally —
use it as a first wall of defence before pushing to GitHub Actions.

```bash
# Build the image once (or after dependency changes)
docker build -t csp-es-ci ci/

# Run all CI builds + software tests + docs
docker run --rm -v $(pwd):/workspace -w /workspace --user $(id -u):$(id -g) csp-es-ci \
  bash ci/run-ci-builds.sh

# Builds only — faster smoke check, skips functional tests
docker run --rm -v $(pwd):/workspace -w /workspace --user $(id -u):$(id -g) csp-es-ci \
  bash ci/run-ci-builds.sh --skip-tests

# Docs only
docker run --rm -v $(pwd):/workspace -w /workspace --user $(id -u):$(id -g) csp-es-ci \
  bash -c "cmake -B build-docs -S doc && cmake --build build-docs"
```

**Not covered locally** (require GitHub Actions CI):
CAN/vcan0 hardware tests, Zephyr SDK builds, FreeRTOS integration tests.

## Supported Platforms

| Platform | Description | Toolchain | Common Use Cases |
|----------|-------------|-----------|------------------|
| `linux` | Native Linux x86_64 | System GCC | Development, testing, server applications |
| `windows` | Windows x86_64 | MinGW cross-compiler | Windows applications, cross-platform support |
| `arm` | ARM Cortex-M7 | arm-none-eabi-gcc | Embedded systems, FreeRTOS, microcontrollers |
| `aarch64` | ARM64 Linux | aarch64-linux-gnu-gcc | ARM64 servers, Go applications, Raspberry Pi |

## Toolchain Files

Each platform uses a dedicated CMake toolchain file that contains all the necessary compiler settings and flags:

- **`contrib/scripts/toolchain-linux.cmake`** - Native Linux builds
- **`contrib/scripts/toolchain-windows.cmake`** - Windows cross-compilation with MinGW
- **`contrib/scripts/toolchain-arm-cortex-m7.cmake`** - ARM Cortex-M7 embedded builds
- **`contrib/scripts/toolchain-arm64.cmake`** - AArch64 Linux cross-compilation

### Customizing Toolchain Settings

You can customize toolchain behavior in two ways:

**1. Environment Variables (recommended for CI/testing):**
```bash
# Use a custom toolchain prefix
export TOOLCHAIN_PREFIX="aarch64-linux-gnu-"
./contrib/scripts/build-and-verify-csp.sh --platform aarch64 ...

# Specify ARM architecture variant
export ARM_ARCH="cortex-m4"
./contrib/scripts/build-and-verify-csp.sh --platform arm ...
```

**2. Modify Toolchain Files (for permanent changes):**
Edit the appropriate toolchain file in `contrib/scripts/` to change compiler flags, add custom settings, or support new toolchain variants.

## Script Options

The `build-and-verify-csp.sh` script supports these options:

```bash
./contrib/scripts/build-and-verify-csp.sh [OPTIONS]

OPTIONS:
    --platform <platform>     Target platform (linux|windows|arm|aarch64)
    --build-dir <path>        Build directory name (e.g., build-linux)
    --target-arch <arch>      Expected target architecture for verification
    --cmake-args "<args>"     Additional CMake arguments (quoted string)
    --toolchain-file <file>   Custom CMake toolchain file (optional)
    --build-target <target>   Build target name (default: csp_es)
    --verbose                 Enable verbose output
    --help                    Show help message
```

## Common Build Scenarios

### Development and Testing
```bash
# Quick development build with samples
./contrib/scripts/build-and-verify-csp.sh \
  --platform linux \
  --build-dir build-dev \
  --target-arch "x86-64" \
  --cmake-args "-DCMAKE_BUILD_TYPE=Debug -DCSP_BUILD_SAMPLES=ON"
```

### Production Release
```bash
# Optimized release build without samples
./contrib/scripts/build-and-verify-csp.sh \
  --platform linux \
  --build-dir build-release \
  --target-arch "x86-64" \
  --cmake-args "-DCMAKE_BUILD_TYPE=Release -DCSP_BUILD_SAMPLES=OFF"
```

### Go Integration (AArch64)
```bash
# Build with Position Independent Code for Go CGO
./contrib/scripts/build-and-verify-csp.sh \
  --platform aarch64 \
  --build-dir build-go \
  --target-arch "aarch64" \
  --cmake-args "-DGOLANG=ON -DCMAKE_BUILD_TYPE=Release -DCSP_BUILD_SAMPLES=OFF"
```

## Architecture Verification

The script automatically verifies that the built libraries match the expected target architecture:

- **Linux/AArch64/ARM**: Uses `readelf` to check ELF headers
- **Windows**: Validates PE32+ format and cross-compilation setup
- **AArch64**: Additional PIC (Position Independent Code) verification for Go compatibility

If verification fails, the script will exit with an error and detailed information about the mismatch.

## Integrating into Your CI Pipeline

The build scripts in this repository are CI-agnostic and can be used with any CI/CD system
(GitHub Actions, GitLab CI, Jenkins, etc.). To add support for a new architecture:

### 1. Create a Toolchain File
Create a new toolchain file in `contrib/scripts/toolchain-<your-arch>.cmake`:

```cmake
# toolchain-your-arch.cmake
set(CMAKE_SYSTEM_NAME YourSystem)
set(CMAKE_SYSTEM_PROCESSOR your_arch)
set(CMAKE_C_COMPILER your-arch-gcc)
set(CMAKE_CXX_COMPILER your-arch-g++)
# Add your specific settings...
```

### 2. Update the Build Script
Add your platform to the auto-detection logic in `build-and-verify-csp.sh`:

```bash
your-arch)
    TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain-your-arch.cmake"
    ;;
```

### 3. Add a CI Job
Invoke `build-and-verify-csp.sh` from your CI pipeline. Example using GitHub Actions syntax:

```yaml
- name: Build for your-arch
  run: |
    ./contrib/scripts/build-and-verify-csp.sh \
      --platform your-arch \
      --build-dir build-your-arch \
      --target-arch "YourArch" \
      --cmake-args "-DCMAKE_BUILD_TYPE=Release"
```

## Troubleshooting

**Missing Cross-Compiler**: Install the required cross-compilation toolchain for your target platform.

**Architecture Mismatch**: Check that you're using the correct `--target-arch` parameter and that your toolchain is properly configured.

**Build Failures**: Use `--verbose` flag to see detailed CMake and build output for debugging.

**Custom Toolchain**: Use `--toolchain-file` to specify a custom toolchain file for specialized build requirements.


# The Cubesat Space Protocol

![CSP](./doc/_images/csp.png)

Cubesat Space Protocol (CSP) is a small protocol stack written in C. CSP
is designed to ease communication between distributed embedded systems
in smaller networks, such as Cubesats. The design follows the TCP/IP
model and includes a transport protocol, a routing protocol and several
MAC-layer interfaces. The core of `libcsp`
includes a router, a connection oriented socket API and
message/connection pools.

The protocol is based on an very lightweight header containing both transport and
network-layer information. Its implementation is designed for, but not
limited to, embedded systems with very limited CPU and memory resources.
The implementation is written in GNU C and is currently ported to run on FreeRTOS, Zephyr
and Linux (POSIX).

The idea is to give sub-system developers of cubesats the same features
of a TCP/IP stack, but without adding the huge overhead of the IP
header. The small footprint and simple implementation allows a small
8-bit system to be fully connected on the network. This allows all
subsystems to provide their services on the same network level, without
any master node required. Using a service oriented architecture has
several advantages compared to the traditional master/slave topology used
on many cubesats.

  - Standardised network protocol: All subsystems can communicate with
    each other (multi-master)
  - Service loose coupling: Services maintain a relationship that
    minimizes dependencies between subsystems
  - Service abstraction: Beyond descriptions in the service contract,
    services hide logic from the outside world
  - Service reusability: Logic is divided into services with the
    intention of promoting reuse.
  - Service autonomy: Services have control over the logic they
    encapsulate.
  - Service Redundancy: Easily add redundant services to the bus
  - Reduces single point of failure: The complexity is moved from a
    single master node to several well defined services on the network

The implementation of `libcsp` is written
with simplicity in mind, but its compile time configuration allows it
to have some rather advanced features as well.

## Features

  - Thread safe Socket API
  - Router task with Quality of Services
  - Connection-oriented operation (RFC 908 and 1151).
  - Connection-less operation (similar to UDP)
  - ICMP-like requests such as ping and buffer status.
  - Loopback interface
  - Very Small Footprint in regards to code and memory required
  - Zero-copy buffer and queue system
  - Modular network interface system
  - OS abstraction, currently ported to: FreeRTOS, Zephyr, Linux
  - Broadcast traffic
  - Promiscuous mode

## Documentation

Technical documentation is available in the [`doc/`](./doc/) folder of this repository.

## Contributing

Thank you for considering contributing to csp_es! We welcome
contributions from the community to help improve and grow the
project. Please take a moment to review our
[contribution guidelines](./CONTRIBUTING.md) before opening a Pull Request!

## Software license

The source code is available under MIT license, see LICENSE for license text
