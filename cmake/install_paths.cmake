include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(PkgConfig REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development)
pkg_check_modules(SYSTEMD libsystemd)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build, options are: None Debug Release."
            FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# If no installation prefix is given manually, install locally.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE STRING
            "The install location"
            FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

if(SYSTEMD_FOUND)
        pkg_get_variable(_SYSTEMD_PREFIX systemd prefix)
        pkg_get_variable(_SYSTEMD_SYSTEMUNITDIR systemd systemdsystemunitdir)
        string(REPLACE "${_SYSTEMD_PREFIX}/" "" SYSTEMD_SYSTEMUNITDIR ${_SYSTEMD_SYSTEMUNITDIR})
else()
    set(SYSTEMD_SYSTEMUNITDIR lib/systemd/system)
endif()

# Projectwise paths
set(PROJECT_PREFIX ${PROJECT_NAME})
set(INSTALL_RUNTIME_DIR ${CMAKE_INSTALL_BINDIR})
set(INSTALL_CONFIG_DIR  ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${PROJECT_NAME})
set(INSTALL_SERVICES_CFG_DIR  ${CMAKE_INSTALL_DATAROOTDIR}/service_cfg)
set(INSTALL_SYSCONFIG_DIR  ${CMAKE_INSTALL_SYSCONFDIR})
set(INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR})
set(INSTALL_ARCHIVE_DIR ${CMAKE_INSTALL_LIBDIR})
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
set(INSTALL_SRC_DIR src/${PROJECT_NAME})
if(Python3_FOUND)
    cmake_path (GET Python3_SITELIB FILENAME THIRD_PARTY_DIR_NAME)
    cmake_path (GET Python3_STDLIB FILENAME PYTHON3_DIR)
    cmake_path (GET Python3_EXECUTABLE FILENAME PYTHON3_EXEC)
    set(INSTALL_PYTHON_DIR ${INSTALL_LIBRARY_DIR}/${PYTHON3_DIR}/${THIRD_PARTY_DIR_NAME})
else()
    message(WARNING "Python3 Development headers not found - using fallback Python install path")
    set(INSTALL_PYTHON_DIR ${INSTALL_LIBRARY_DIR}/python/dist-packages)
endif()
set(INSTALL_SYSTEMD_SERVICE ${SYSTEMD_SYSTEMUNITDIR})
