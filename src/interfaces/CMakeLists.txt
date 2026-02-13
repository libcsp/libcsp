target_sources(csp PRIVATE
  csp_if_lo.c
  csp_if_kiss.c
  csp_if_i2c.c
  csp_if_tun.c
  csp_if_can.c
  csp_if_can_pbuf.c
  csp_if_eth.c
  csp_if_eth_pbuf.c
  )

if(LIBZMQ_FOUND)
  add_library(if_zmq OBJECT csp_if_zmqhub.c)
  target_include_directories(if_zmq PRIVATE
    ${csp_inc}
    ${LIBZMQ_INCLUDE_DIRS})
  target_link_libraries(if_zmq PRIVATE csp_common ${LIBZMQ_LIBRARIES})
  target_link_libraries(csp PRIVATE if_zmq)
  if(BUILD_SHARED_LIBS)
    set_property(TARGET if_zmq PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

if(HAVE_SYS_SOCKET_H AND HAVE_ARPA_INET_H)
  target_sources(csp PRIVATE csp_if_udp.c)
endif()
