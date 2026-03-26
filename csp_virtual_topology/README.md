# CSP Virtual Topology Simulator

A standalone tool for simulating and testing CSP (Cubesat Space Protocol) network topologies using virtual nodes and ZMQ transport.

## Overview

This project provides a JSON-configurable virtual node system that enables testing of various CSP network topologies. It uses ZMQ as the underlying transport for all interfaces, allowing simulation of complex multi-hop routing scenarios, hot-redundant systems, and multi-bus architectures.

## Features

- **JSON-based Configuration**: Define entire network topologies in JSON
- **Virtual CSP Nodes**: Simulate multiple CSP nodes as separate processes
- **ZMQ Transport**: All interfaces (RF, CAN, I2C) simulated via ZMQ
- **Multi-hop Routing**: Test complex routing scenarios
- **Hot-redundancy Support**: Simulate redundant radios and deduplication
- **Packet Monitoring**: Comprehensive packet transit logging
- **Topology Visualization**: Auto-generate Graphviz diagrams
- **Multiple Bus Types**: Simulate RF, CAN, Sensor CAN, I2C buses

## Dependencies

### Required

- **csp_es**: CSP library (must be installed)
- **cJSON**: JSON parsing library
  - Ubuntu/Debian: `sudo apt-get install libcjson-dev`
  - Fedora/RHEL: `sudo dnf install cjson-devel`
- **libzmq**: ZeroMQ messaging library
  - Ubuntu/Debian: `sudo apt-get install libzmq3-dev`
  - Fedora/RHEL: `sudo dnf install zeromq-devel`

### Optional

- **Python 3**: For topology launcher and visualization
- **graphviz**: For topology visualization
  - System: `sudo apt-get install graphviz`
  - Python: `pip install graphviz`

## Building

```bash
# Create build directory
mkdir build && cd build

# Configure (csp_es must be installed or in CMAKE_PREFIX_PATH)
cmake ..

# Build
make

# Install (optional)
sudo make install
```

## Quick Start

### 1. Create a Topology Configuration

See `topologies/examples/linear_3node.json` for a simple example.

### 2. Run the Topology

```bash
# Launch topology (web dashboard starts automatically on http://localhost:9999)
./tools/topology_launcher.py topologies/examples/linear_3node.json

# Open http://localhost:9999 in your browser to:
# - View topology visualization
# - Monitor real-time logs
# - Send ping commands
# - Run traceroute tests
```

## Project Structure

```
csp_virtual_topology/
├── src/                    # Source code
│   ├── config_parser.c/h   # JSON configuration parser
│   └── csp_virtual_node.c  # Virtual node executable
├── tools/                  # Python tools
│   └── topology_launcher.py
├── topologies/             # Topology configurations
│   ├── examples/           # Example topologies
│   └── schema.json         # JSON schema
├── tests/                  # Unit tests
└── CMakeLists.txt          # Build configuration
```

## Documentation

See the main implementation plan: `../CSP_VIRTUAL_NODE_IMPLEMENTATION_PLAN.md`

## License

Same as csp_es library.

## Author

Part of the csp_es ecosystem.

