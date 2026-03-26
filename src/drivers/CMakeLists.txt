target_sources(csp_es PRIVATE usart/usart_kiss.c)

if(LIBSOCKETCAN_FOUND)
  add_library(driver_can OBJECT can/linux/socketcan/can_socketcan.c)
  target_include_directories(driver_can PRIVATE
    ${csp_inc}
    ${LIBSOCKETCAN_INCLUDE_DIRS})
  target_link_libraries(driver_can PRIVATE csp_common
    ${LIBSOCKETCAN_LIBRARIES})
  target_link_libraries(csp_es PRIVATE driver_can)
  if(BUILD_SHARED_LIBS)
    set_property(TARGET driver_can PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

if(CSP_POSIX)
  target_sources(csp_es PRIVATE usart/usart_linux.c)
  target_sources(csp_es PRIVATE eth/eth_linux.c)
elseif(CSP_ZEPHYR)
  target_sources(csp_es PRIVATE usart/usart_zephyr.c)
elseif (CSP_WINDOWS)
  target_sources(csp_es PRIVATE usart/usart_windows.c)

  # Platform-specific CAN drivers are not shipped in the public distribution.
  # Provide implementations via CSP_EXTRA_DRIVER_DIRS (see below).
endif()

# Generic extension point: consumers supply additional driver directories.
# Each directory must contain a CMakeLists.txt that builds an OBJECT library
# and calls target_link_libraries(csp_es PRIVATE <driver_target>).
# Each driver's CMakeLists.txt is responsible for its own platform guard.
foreach(_drv_dir IN LISTS CSP_EXTRA_DRIVER_DIRS)
    get_filename_component(_drv_name "${_drv_dir}" NAME)
    add_subdirectory("${_drv_dir}" "${CMAKE_CURRENT_BINARY_DIR}/extra/${_drv_name}")
endforeach()

if(HAVE_ZEPHYR_CAN)
  target_sources(csp_es PRIVATE can/zephyr/can_zephyr.c)
endif()
