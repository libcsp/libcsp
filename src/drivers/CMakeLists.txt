target_sources(libcsp PRIVATE usart/usart_kiss.c)

if(LIBSOCKETCAN_FOUND)
  add_library(driver_can OBJECT can/can_socketcan.c)
  target_include_directories(driver_can PRIVATE
    ${csp_inc}
    ${LIBSOCKETCAN_INCLUDE_DIRS})
  target_link_libraries(driver_can PRIVATE
    ${LIBSOCKETCAN_LIBRARIES})
  target_link_libraries(libcsp PRIVATE driver_can)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(libcsp PRIVATE usart/usart_linux.c)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_sources(libcsp PRIVATE usart/usart_windows.c)
endif()
