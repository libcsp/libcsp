if(CONFIG_LIBCSP)
  set(CSP_SYSTEM_NAME "Zephyr")
  set(CMAKE_BUILD_TYPE None)

  set(CSP_PRINT_STDIO OFF)

  # check_include_file() doesn't work in Zephyr module
  # https://github.com/zephyrproject-rtos/zephyr/issues/31193
  # predefine it
  set(HAVE_SYS_SOCKET_H OFF)

  # Cache entries. see the top level CMakeLists.txt
  set(CSP_QFIFO_LEN        CONFIG_CSP_QFIFO_LEN CACHE STRING "")
  set(CSP_PORT_MAX_BIND    CONFIG_CSP_PORT_MAX_BIND CACHE STRING "")
  set(CSP_CONN_RXQUEUE_LEN CONFIG_CSP_CONN_RXQUEUE_LEN CACHE STRING "")
  set(CSP_CONN_MAX         CONFIG_CSP_CONN_MAX CACHE STRING "")
  set(CSP_BUFFER_SIZE      CONFIG_CSP_BUFFER_SIZE CACHE STRING "")
  set(CSP_BUFFER_COUNT     CONFIG_CSP_BUFFER_COUNT CACHE STRING "")
  set(CSP_RDP_MAX_WINDOW   CONFIG_CSP_RDP_MAX_WINDOW CACHE STRING "")
  set(CSP_RTABLE_SIZE      CONFIG_CSP_RTABLE_SIZE CACHE STRING "")

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
