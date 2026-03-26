# Changelog

All notable changes to csp_es will be documented in this file.

This file covers changes specific to the EnduroSat fork (`csp_es`). For upstream
libcsp changes, see [github.com/libcsp/libcsp](https://github.com/libcsp/libcsp).

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [1.0.0-rc.1]

### Added

- **CSP Virtual Topology Simulator** — Python/C/JavaScript tool for simulating
  multi-node CSP networks without physical hardware. Supports topology JSON
  configuration, ZMQ-based inter-node communication, and a web UI.
- **Multi-architecture build scripts** — `contrib/scripts/build-and-verify-csp.sh`
  and accompanying CMake toolchain files for Linux (x86-64), Windows (MinGW
  cross-compilation), ARM Cortex-M7 (embedded/FreeRTOS), and AArch64.
- **Go interoperability** — Position Independent Code (PIC) support via
  `-DGOLANG=ON` CMake option, enabling use of csp_es as a CGO library in Go
  applications targeting AArch64.
- **External driver build-time support** — CMake infrastructure for integrating
  external (proprietary or platform-specific) drivers at build time without
  modifying the core library.
- **CMake install and packaging support** — `cmake --install` target,
  `find_package`-compatible export, and CPack Debian package generation.
- **Submodule build mode** — `SUBMODULE_EXTERNAL_LIBCSP_ES_VERSION` option
  allowing csp_es to be consumed as a CMake subdirectory (submodule) by a
  parent project.
- **GitHub Actions CI** — Multi-platform build matrix covering Linux (GCC 10–14,
  Clang), FreeRTOS, Zephyr, and Python bindings; ABI compatibility checker.
- **Windows architecture support** — Reorganized Windows OS abstraction layer
  into `src/arch/windows/` (clock, queue, rand, semaphore, system, thread, time)
  for better repo navigability. Replaces the earlier `contrib/windows/` layout.
- **Windows USART driver** — `src/drivers/usart/usart_windows.c`.
- **Traceroute service** — New `csp_traceroute` functionality
  (`include/csp/csp_traceroute.h`, `src/csp_traceroute.c`).
- **RF interface** — New `csp_if_rf` interface
  (`include/csp/interfaces/csp_if_rf.h`, `src/interfaces/csp_if_rf.c`).
- **Community files** — `CODE_OF_CONDUCT.md` and `CONTRIBUTING.md`.

### Changed

- Improved portability of core implementation to support 16-bit MCUs.
- Improved CMake target system detection for cross-compilation scenarios.
- Minimum CMake version raised to 3.20.
- **Driver directory reorganization** — CAN drivers split into platform-specific
  subdirectories (`src/drivers/can/linux/socketcan/`, `src/drivers/can/zephyr/`).
- **Zephyr module relocation** — Moved from `zephyr/` to `contrib/zephyr/`.
- **Debug/trace output** — Minor improvements to tracing and debug output across
  core modules; core logic kept close to upstream.
- **Ethernet interface** — `csp_if_eth` not yet fully aligned to the latest
  upstream fork.
