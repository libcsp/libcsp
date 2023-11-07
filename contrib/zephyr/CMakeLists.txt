if(CONFIG_LIBCSP)
  set(CMAKE_SYSTEM_NAME "Zephyr")
  set(CMAKE_BUILD_TYPE None)

  set(CSP_PRINT_STDIO OFF)

  # check_include_file() doesn't work in Zephyr module
  # https://github.com/zephyrproject-rtos/zephyr/issues/31193
  # predefine it
  set(HAVE_SYS_SOCKET_H OFF)

  if(CONFIG_CSP_USE_RTABLE)
    set(CSP_USE_RTABLE ON)
  endif()

  if(CONFIG_CSP_HAVE_CAN)
    set(HAVE_ZEPHYR_CAN ON)
  endif()

  add_subdirectory(../.. build)

  zephyr_interface_library_named(libcsp)
  target_include_directories(libcsp INTERFACE ${ZEPHYR_CURRENT_MODULE_DIR}/include)
  target_include_directories(libcsp INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/build/include)

  zephyr_append_cmake_library(csp)
  target_link_libraries(csp PUBLIC zephyr_interface)
endif()
