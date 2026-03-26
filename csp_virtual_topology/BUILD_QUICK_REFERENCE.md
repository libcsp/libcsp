# Build Script Quick Reference

## One-Liners

```bash
# Debug build (default)
./build.sh

# Release build
./build.sh --release

# Clean Release build
./build.sh --clean --release

# Build with 8 parallel jobs
./build.sh -j 8

# Build and install to ~/.local
./build.sh --release --install --prefix ~/.local

# Verbose Release build
./build.sh --release --verbose

# Build without colors (for CI/CD)
./build.sh --no-color
```

## Common Workflows

### Development Build
```bash
cd csp_virtual_topology
./build.sh
```
- Debug symbols included
- No optimizations
- Faster compilation

### Production Build
```bash
./build.sh --clean --release -j 8
```
- Optimized for performance
- Smaller binaries
- Suitable for deployment

### Install Locally
```bash
./build.sh --release --install --prefix ~/.local
```
- No sudo required
- Installs to user directory
- Good for testing

### Install System-wide
```bash
./build.sh --release --install
# Requires sudo for /usr/local
```

### Rebuild After Changes
```bash
./build.sh --release
```
- Incremental build
- Only recompiles changed files
- Faster than clean build

### Full Clean Rebuild
```bash
./build.sh --clean --release -j 8
```
- Removes all build artifacts
- Rebuilds everything from scratch
- Useful for troubleshooting

## Option Combinations

| Use Case | Command |
|----------|---------|
| Quick debug build | `./build.sh` |
| Quick release build | `./build.sh --release` |
| Full clean release | `./build.sh --clean --release` |
| Fast parallel build | `./build.sh -j 16` |
| Install locally | `./build.sh --install --prefix ~/.local` |
| Verbose debugging | `./build.sh --verbose` |
| CI/CD pipeline | `./build.sh --clean --release --no-color` |
| Maximum performance | `./build.sh --clean --release -j $(nproc)` |

## Build Output Files

After building, find binaries in `build/`:

```
build/
├── csp_virtual_node          # Virtual CSP node executable
├── zmqproxy                  # ZMQ message broker
└── libconfig_parser.a        # Configuration parser library
```

## Checking Build Status

```bash
# List build artifacts
ls -lh build/csp_virtual_node build/zmqproxy build/libconfig_parser.a

# Check binary info
file build/csp_virtual_node
ldd build/csp_virtual_node    # Show dependencies
```

## Troubleshooting

```bash
# Check dependencies
./build.sh --help

# Verbose output for errors
./build.sh --verbose

# Clean and rebuild
./build.sh --clean --release --verbose

# Check CMake cache
cat build/CMakeCache.txt | grep -i error
```

## Environment Variables

```bash
# Use specific compiler
CC=gcc CXX=g++ ./build.sh

# Set custom flags
CFLAGS="-O3 -march=native" ./build.sh --release

# Parallel jobs from CPU count
./build.sh -j $(nproc)
```

## Installation Verification

```bash
# After installation
which csp_virtual_node
which zmqproxy

# Check installed files
ls -la ~/.local/bin/csp_virtual_node
ls -la ~/.local/bin/zmqproxy
```

## See Also

- [BUILD.md](BUILD.md) - Comprehensive build guide
- [README.md](README.md) - Project overview
- `./build.sh --help` - Full help text

