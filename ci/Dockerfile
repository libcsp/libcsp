# CI Build Environment for csp_es
#
# Provides all dependencies to replicate CI build jobs locally,
# as a first wall of defence before pushing to GitHub Actions.
# Covers compiled builds, software tests, ABI checks, and Sphinx docs.
#
# Usage:
#   # Build image (once, or after dependency changes)
#   docker build -t csp-es-ci ci/
#
#   # Run all CI builds + software tests + docs
#   docker run --rm -v $(pwd):/workspace -w /workspace --user $(id -u):$(id -g) csp-es-ci \
#     bash ci/run-ci-builds.sh
#
#   # Builds only (faster smoke check, no functional tests)
#   docker run --rm -v $(pwd):/workspace -w /workspace --user $(id -u):$(id -g) csp-es-ci \
#     bash ci/run-ci-builds.sh --skip-tests
#
#   # Docs only
#   docker run --rm -v $(pwd):/workspace -w /workspace --user $(id -u):$(id -g) csp-es-ci \
#     bash -c "cmake -B build-docs -S doc && cmake --build build-docs"
#
# Python venvs (managed by uv, under /opt/venv/):
#   /opt/venv/py3.9   — Python 3.9  + binding test deps
#   /opt/venv/py3.10  — Python 3.10 + binding test deps
#   /opt/venv/py3.11  — Python 3.11 + binding test deps + Sphinx/docs tools (default python3)
#   /opt/venv/py3.12  — Python 3.12 + binding test deps + meson>=1.3
#
# The default python3 on the PATH points to py3.11 (system Ubuntu default).
# To run with a specific version:
#   source /opt/venv/py3.9/bin/activate
# Or directly:
#   /opt/venv/py3.12/bin/python3 examples/buildall.py --build-system=meson
#
# What is NOT supported in this container (requires external setup):
#   - CAN/vcan0 tests: hardware-dependent, stay in GitHub Actions CI
#   - Zephyr builds: ~2GB SDK installed via GitHub Action only
#   - FreeRTOS integration test: requires checkout of libcsp/libcsp-freertos
#     and FreeRTOS/FreeRTOS-Kernel at runtime (external repos)

FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# Locale setup
RUN apt-get update && \
    apt-get install -y --no-install-recommends locales locales-all && \
    locale-gen en_US.UTF-8

ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US.UTF-8

# Core build tools
RUN apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    curl \
    wget \
    ca-certificates

# GCC versions available on Ubuntu 22.04: 10, 11, 12
# (gcc-13/14 require ubuntu-24.04; covered by GitHub Actions CI)
RUN apt-get install -y --no-install-recommends \
    gcc-10 g++-10 \
    gcc-11 g++-11 \
    gcc-12 g++-12 \
    clang-14

# Cross-compilation toolchains
RUN apt-get install -y --no-install-recommends \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    mingw-w64

# Library dependencies
# libsocketcan-dev: needed at build time by meson/waf for CAN driver compilation.
# CAN runtime tests (vcan0) remain excluded — those require the kernel module and stay in GitHub Actions CI.
RUN apt-get install -y --no-install-recommends \
    libzmq3-dev \
    libyaml-dev \
    libsocketcan-dev

# Software testing tools (socat for KISS PTY tests; iproute2 excluded — CAN-only)
RUN apt-get install -y --no-install-recommends \
    socat

# ABI checker tools (from abi-checker.yml)
RUN apt-get install -y --no-install-recommends \
    abi-compliance-checker \
    abi-dumper

# Documentation / Python binding tools — system Python + libclang headers
RUN apt-get install -y --no-install-recommends \
    libclang-dev \
    python3 \
    python3-dev

# Build system: meson via apt (used as fallback; uv venvs provide versioned meson)
RUN apt-get install -y --no-install-recommends \
    meson

# Misc utilities
RUN apt-get install -y --no-install-recommends \
    tree \
    file \
    binutils

# Clean up apt cache
RUN apt-get clean && rm -rf /var/lib/apt/lists/*

# Install uv (fast Python package/venv manager)
RUN curl -LsSf https://astral.sh/uv/install.sh | sh
ENV PATH="/root/.local/bin:${PATH}"
# Store uv-managed Pythons in a world-accessible location so --user runs work
ENV UV_PYTHON_INSTALL_DIR=/opt/uv-python

# Python venvs — one per version tested in CI (3.9, 3.10, 3.11, 3.12)
# Each venv gets the Python bindings test deps.
# py3.11 also gets the Sphinx/docs stack (clang==14 requires libclang-dev above).
# py3.12 gets meson>=1.3 (required because distutils was removed in 3.12).

# Binding-test requirements shared across all versions
ENV BINDING_REQS="meson>=1.3"

RUN uv venv --python 3.9  /opt/venv/py3.9  && \
    uv pip install --python /opt/venv/py3.9/bin/python  ${BINDING_REQS}

RUN uv venv --python 3.10 /opt/venv/py3.10 && \
    uv pip install --python /opt/venv/py3.10/bin/python ${BINDING_REQS}

RUN uv venv --python 3.11 /opt/venv/py3.11 && \
    uv pip install --python /opt/venv/py3.11/bin/python ${BINDING_REQS} \
        "sphinx==5.3.0" \
        myst-parser \
        sphinx-rtd-theme \
        sphinx_c_autodoc \
        sphinxcontrib-apidoc \
        sphinx_design \
        sphinx_git \
        sphinx_copybutton \
        "clang==14" \
        pygit2 \
        gitlint

RUN uv venv --python 3.12 /opt/venv/py3.12 && \
    uv pip install --python /opt/venv/py3.12/bin/python ${BINDING_REQS}

# Make venvs world-readable/executable so --user $(id -u):$(id -g) runs work
RUN chmod -R a+rX /opt/venv

# Make the docs/default venv (py3.11) the system-wide python3 fallback
# so that existing scripts using bare `python3` keep working.
ENV PATH="/opt/venv/py3.11/bin:${PATH}"

WORKDIR /workspace
CMD ["/bin/bash"]
