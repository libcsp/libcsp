# toolchain-linux.cmake

# Specify the target system details for native Linux compilation.
set(CMAKE_SYSTEM_NAME Linux)             # Target system is Linux.
set(CMAKE_SYSTEM_PROCESSOR x86_64)       # Target architecture is x86_64.

# For native compilation, we typically use the system's default compilers
# But we can still allow override via environment variables for consistency

# Determine the toolchain prefix, defaulting to empty (system default) if not set by the user.
if(DEFINED ENV{TOOLCHAIN_PREFIX})
  set(TOOLCHAIN_PREFIX "$ENV{TOOLCHAIN_PREFIX}")
  message(STATUS "Using custom TOOLCHAIN_PREFIX=${TOOLCHAIN_PREFIX}")
  
  # Set the C and C++ compilers to the prefixed tools.
  set(CMAKE_C_COMPILER   "${TOOLCHAIN_PREFIX}gcc")
  set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++")
  
  # Set additional tools required during the build process.
  set(CMAKE_LINKER       "${TOOLCHAIN_PREFIX}ld")
  set(CMAKE_AR           "${TOOLCHAIN_PREFIX}ar")
  set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}as")
  set(CMAKE_RANLIB       "${TOOLCHAIN_PREFIX}ranlib")
  set(CMAKE_STRIP        "${TOOLCHAIN_PREFIX}strip")
  
  # Manually specify the tools that CMake does not automatically configure.
  set(OBJCOPY            "${TOOLCHAIN_PREFIX}objcopy")
  set(OBJDUMP            "${TOOLCHAIN_PREFIX}objdump")
  set(NM                 "${TOOLCHAIN_PREFIX}nm")
  set(READELF            "${TOOLCHAIN_PREFIX}readelf")
  
  # CMake usually handles preprocessing internally, but you can define the path explicitly if needed.
  set(CPP                "${TOOLCHAIN_PREFIX}cpp")
else()
  message(STATUS "Using system default compilers for Linux native build")
  # Let CMake use system defaults - no explicit compiler settings needed
endif()

# Linux-specific settings (if any)
# For native builds, we typically don't need special find root paths
