# CSP Virtual Topology - UV Setup Guide

This guide explains how to set up and run the CSP Virtual Topology using `uv`, a fast Python package manager and project manager.

## What is uv?

`uv` is a fast, reliable Python package installer and project manager written in Rust. It's compatible with pip and provides better performance and dependency resolution.

**Installation:**
```bash
# macOS/Linux
curl -LsSf https://astral.sh/uv/install.sh | sh

# Windows (PowerShell)
powershell -c "irm https://astral.sh/uv/install.ps1 | iex"

# Or via package managers
brew install uv              # macOS
sudo apt-get install uv      # Ubuntu/Debian (if available)
```

## Quick Start with uv

### Option 1: Using requirements.txt (Simplest)

```bash
cd csp_virtual_topology

# Install dependencies
uv pip install -r requirements.txt

# Run the topology launcher
uv run python tools/topology_launcher.py topologies/examples/satellite_hot_redundant.json
```

### Option 2: Using Virtual Environment

```bash
cd csp_virtual_topology

# Create a virtual environment
uv venv

# Activate it
source .venv/bin/activate  # Linux/macOS
# or
.venv\Scripts\activate     # Windows

# Install dependencies
uv pip install -r requirements.txt

# Run the topology launcher
python tools/topology_launcher.py topologies/examples/satellite_hot_redundant.json
```

### Option 3: Using pyproject.toml (Recommended)

```bash
cd csp_virtual_topology

# Create and activate virtual environment
uv venv
source .venv/bin/activate

# Install project with dependencies
uv pip install -e .

# Run the topology launcher
csp-topology-launcher topologies/examples/satellite_hot_redundant.json
```

## Common uv Commands

### Dependency Management

```bash
# Install all dependencies
uv pip install -r requirements.txt

# Install specific package
uv pip install pyzmq

# Install with specific version
uv pip install "flask>=3.0.0,<4.0.0"

# Upgrade a package
uv pip install --upgrade pyzmq

# List installed packages
uv pip list

# Show package information
uv pip show flask
```

### Running Python Scripts

```bash
# Run a script with dependencies
uv run python tools/topology_launcher.py topologies/examples/linear_3node.json

# Run with specific Python version
uv run --python 3.11 python tools/topology_launcher.py topologies/examples/linear_3node.json

# Run with additional dependencies
uv run --with pytest python -m pytest tests/
```

### Virtual Environment Management

```bash
# Create virtual environment
uv venv

# Create with specific Python version
uv venv --python 3.11

# Activate (Linux/macOS)
source .venv/bin/activate

# Activate (Windows)
.venv\Scripts\activate

# Deactivate
deactivate
```

## Development Setup

For development with testing and linting:

```bash
cd csp_virtual_topology

# Create virtual environment
uv venv
source .venv/bin/activate

# Install project with dev dependencies
uv pip install -e ".[dev]"

# Run tests
uv run pytest

# Format code
uv run black tools/

# Lint code
uv run flake8 tools/

# Type check
uv run mypy tools/
```

## Running the Topology Launcher

### Basic Usage

```bash
# Run topology (web dashboard starts automatically on http://localhost:9999)
uv run python tools/topology_launcher.py topologies/examples/satellite_hot_redundant.json

# Run with custom web port
uv run python tools/topology_launcher.py topologies/examples/satellite_hot_redundant.json --web-port 8080

# Generate visualization file
uv run python tools/topology_launcher.py topologies/examples/satellite_hot_redundant.json --visualize
```

### Advanced Usage

```bash
# Run with custom build directory
uv run python tools/topology_launcher.py topologies/examples/linear_3node.json --build-dir /path/to/build

# Run with verbose output
uv run python tools/topology_launcher.py topologies/examples/linear_3node.json --verbose

# Run with custom log directory
uv run python tools/topology_launcher.py topologies/examples/linear_3node.json --log-dir /tmp/csp_logs
```

## Troubleshooting

### uv not found
Make sure uv is installed and in your PATH:
```bash
uv --version
```

### Permission denied on Linux/macOS
```bash
chmod +x ~/.cargo/bin/uv
```

### Virtual environment issues
```bash
# Remove and recreate virtual environment
rm -rf .venv
uv venv
source .venv/bin/activate
uv pip install -r requirements.txt
```

### Dependency conflicts
```bash
# Clear pip cache and reinstall
uv pip cache purge
uv pip install --force-reinstall -r requirements.txt
```

## Comparison: pip vs uv

| Feature | pip | uv |
|---------|-----|-----|
| Speed | Slower | Much faster |
| Dependency resolution | Basic | Advanced |
| Lock files | No | Yes (planned) |
| Python version management | No | Yes |
| Installation | Standard | Faster |
| Compatibility | Standard | pip-compatible |

## Next Steps

1. **Build the C components:**
   ```bash
   cd csp_virtual_topology
   ./build.sh --release
   ```

2. **Install Python dependencies:**
   ```bash
   uv pip install -r requirements.txt
   ```

3. **Run the topology:**
   ```bash
   uv run python tools/topology_launcher.py topologies/examples/satellite_hot_redundant.json
   ```

4. **Access the web dashboard:**
   The web dashboard automatically starts on http://localhost:9999
   Open it in your browser to view topology, logs, and run tests

## See Also

- [uv Documentation](https://docs.astral.sh/uv/)
- [README.md](README.md) - Project overview
- [BUILD.md](BUILD.md) - Build system guide
- [topology_launcher.py](tools/topology_launcher.py) - Main orchestrator

