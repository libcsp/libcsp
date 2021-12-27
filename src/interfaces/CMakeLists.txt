target_sources(libcsp PRIVATE
  csp_if_lo.c
  csp_if_kiss.c
  csp_if_i2c.c
  csp_if_tun.c
  )

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Zephyr")
  target_sources(libcsp PRIVATE
    csp_if_can.c
    csp_if_can_pbuf.c
    )
endif()

if(LIBZMQ_FOUND)
  add_library(if_zmq OBJECT csp_if_zmqhub.c)
  target_include_directories(if_zmq PRIVATE
    ${csp_inc}
    ${LIBZMQ_INCLUDE_DIRS})
  target_link_libraries(if_zmq PRIVATE ${LIBZMQ_LIBRARIES})
  target_link_libraries(libcsp PRIVATE if_zmq)
  if(BUILD_SHARED_LIBS)
    set_property(TARGET driver_can PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

if(HAVE_SYS_SOCKET_H AND HAVE_ARPA_INET_H)
  target_sources(libcsp PRIVATE csp_if_udp.c)
endif()
