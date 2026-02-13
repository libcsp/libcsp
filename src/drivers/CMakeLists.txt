target_sources(csp PRIVATE usart/usart_kiss.c)

if(LIBSOCKETCAN_FOUND)
  add_library(driver_can OBJECT can/can_socketcan.c)
  target_include_directories(driver_can PRIVATE
    ${csp_inc}
    ${LIBSOCKETCAN_INCLUDE_DIRS})
  target_link_libraries(driver_can PRIVATE csp_common
    ${LIBSOCKETCAN_LIBRARIES})
  target_link_libraries(csp PRIVATE driver_can)
  if(BUILD_SHARED_LIBS)
    set_property(TARGET driver_can PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

if(CSP_POSIX)
  target_sources(csp PRIVATE usart/usart_linux.c)
  target_sources(csp PRIVATE eth/eth_linux.c)
elseif(CSP_ZEPHYR)
  target_sources(csp PRIVATE usart/usart_zephyr.c)
endif()

if(HAVE_ZEPHYR_CAN)
  target_sources(csp PRIVATE can/can_zephyr.c)
endif()
