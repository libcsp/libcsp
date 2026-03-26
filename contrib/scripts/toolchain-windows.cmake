# toolchain-windows.cmake

# Specify the target system details for cross-compilation to Windows.
set(CMAKE_SYSTEM_NAME Windows)           # Target system is Windows.
set(CMAKE_SYSTEM_PROCESSOR x86_64)       # Target architecture is x86_64.

# Determine the toolchain prefix, defaulting to "x86_64-w64-mingw32-" if not set by the user.
if(DEFINED ENV{TOOLCHAIN_PREFIX})
  set(TOOLCHAIN_PREFIX "$ENV{TOOLCHAIN_PREFIX}")
else()
  set(TOOLCHAIN_PREFIX "x86_64-w64-mingw32-")
  message(STATUS "Using default TOOLCHAIN_PREFIX=${TOOLCHAIN_PREFIX}")
endif()

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

# Windows-specific settings
set(CMAKE_RC_COMPILER  "${TOOLCHAIN_PREFIX}windres")

# Set the find root path for libraries and includes
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
