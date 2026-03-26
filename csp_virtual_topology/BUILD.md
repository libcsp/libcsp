# CSP Virtual Node Infrastructure Build Guide

This document describes how to build the CSP Virtual Node Infrastructure using the provided build script.

## Quick Start

### Build in Debug Mode (Default)
```bash
cd csp_virtual_topology
./build.sh
```

### Build in Release Mode
```bash
./build.sh --release
```

### Clean Build
```bash
./build.sh --clean --release
```

### Build and Install
```bash
./build.sh --release --install --prefix ~/.local
```

## Build Script Options

The `build.sh` script supports the following options:

| Option | Description | Default |
|--------|-------------|---------|
| `-h, --help` | Show help message | - |
| `-c, --clean` | Clean build directory before building | false |
| `-r, --release` | Build in Release mode | Debug |
| `-j, --jobs N` | Number of parallel build jobs | 4 |
| `-i, --install` | Install after building | false |
| `-p, --prefix PATH` | Installation prefix | /usr/local |
| `-v, --verbose` | Verbose build output | false |
| `--no-color` | Disable colored output | false |

## Build Modes

### Debug Mode (Default)
- Includes debug symbols
- No optimizations
- Useful for development and debugging
- Larger binary size

```bash
./build.sh
```

### Release Mode
- Optimized for performance
- Smaller binary size
- Suitable for production

```bash
./build.sh --release
```

## Parallel Build Jobs

Control the number of parallel compilation jobs:

```bash
./build.sh -j 8    # Use 8 parallel jobs
./build.sh -j 1    # Single-threaded build
```

## Installation

### System-wide Installation
```bash
./build.sh --release --install
# Requires sudo for /usr/local
```

### Local Installation
```bash
./build.sh --release --install --prefix ~/.local
# No sudo required
```

### Custom Installation Path
```bash
./build.sh --release --install --prefix /opt/csp
```

## Build Output

The script provides colored output with status indicators:

- ✓ Success (green)
- ✗ Error (red)
- ⚠ Warning (yellow)
- ℹ Information (blue)

Disable colors with `--no-color` for CI/CD environments:

```bash
./build.sh --no-color
```

## Verbose Output

For detailed build information:

```bash
./build.sh --verbose
```

This shows:
- Full CMake configuration output
- Complete compiler commands
- All warnings and errors

## Build Artifacts

After a successful build, the following binaries are created in the `build/` directory:

- `csp_virtual_node` - Virtual CSP node executable
- `zmqproxy` - ZMQ message broker
- `libconfig_parser.a` - Configuration parser library

## Dependency Checking

The script automatically checks for required dependencies:

- **cmake** - Build system
- **ninja** or **make** - Build tool
- **pkg-config** - Package configuration utility
- **libcjson-dev** - JSON parsing library
- **libzmq3-dev** - ZeroMQ messaging library

### Installing Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get install cmake ninja-build pkg-config libcjson-dev libzmq3-dev
```

#### Fedora/RHEL
```bash
sudo dnf install cmake ninja-build pkgconfig cjson-devel zeromq-devel
```

## Build Modes

### Integrated Build (Default)
When run from `csp_virtual_topology/`, the script detects the main CSP project and builds everything together:

```bash
cd csp_virtual_topology
./build.sh
```

This builds:
- All CSP examples
- All CSP samples
- Virtual node infrastructure
- All dependencies

### Standalone Build
If csp_es is installed system-wide, you can build just the virtual node infrastructure:

```bash
cd csp_virtual_topology
./build.sh
# Requires csp_es to be installed
```

## Troubleshooting

### CMake Not Found
```
✗ cmake not found. Install with: sudo apt-get install cmake
```

Install CMake:
```bash
sudo apt-get install cmake
```

### Missing Dependencies
```
✗ libcjson not found. Install with: sudo apt-get install libcjson-dev
```

Install the missing library:
```bash
sudo apt-get install libcjson-dev
```

### Build Fails
Enable verbose output to see detailed error messages:

```bash
./build.sh --verbose
```

### Permission Denied
Make the script executable:

```bash
chmod +x build.sh
```

## Examples

### Complete Release Build with Installation
```bash
./build.sh --clean --release -j 8 --install --prefix ~/.local
```

### Debug Build with Verbose Output
```bash
./build.sh --verbose
```

### Quick Rebuild (No Clean)
```bash
./build.sh --release
```

### Build with Custom Job Count
```bash
./build.sh --release -j 16
```

## Build Summary

After each build, the script displays a summary showing:

- Build type (Debug/Release)
- Build directory
- Build tool used (ninja/make)
- Number of parallel jobs
- Installation prefix (if installing)
- Location of built binaries

Example output:
```
==== Build Summary ====
ℹ Build Type: Release
ℹ Build Directory: build
ℹ Build Tool: ninja
ℹ Parallel Jobs: 4
ℹ Installation Prefix: /usr/local

ℹ Binaries:
✓ csp_virtual_node: build/csp_virtual_node
✓ zmqproxy: build/zmqproxy
✓ libconfig_parser.a: build/libconfig_parser.a

✓ Build process completed successfully!
```

## Next Steps

After building, you can:

1. **Run the topology launcher:**
   ```bash
   ./tools/topology_launcher.py topologies/examples/satellite_hot_redundant.json
   ```

2. **Test connectivity:**
   ```bash
   ./build/csp_virtual_node --help
   ```

3. **View topology configurations:**
   ```bash
   ls -la topologies/examples/
   ```

## See Also

- [README.md](README.md) - Project overview
- [CSP_VIRTUAL_NODE_IMPLEMENTATION_PLAN.md](CSP_VIRTUAL_NODE_IMPLEMENTATION_PLAN.md) - Implementation details
- [topologies/schema.json](topologies/schema.json) - Topology configuration schema

