target_sources(libcsp PRIVATE
  csp_if_lo.c
  csp_if_can.c
  csp_if_can_pbuf.c
  csp_if_kiss.c
  )

if(LIBZMQ_FOUND)
  add_library(if_zmq OBJECT csp_if_zmqhub.c)
  target_include_directories(if_zmq PRIVATE
    ${csp_inc}
    ${LIBZMQ_INCLUDE_DIRS})
  target_link_libraries(if_zmq PRIVATE ${LIBZMQ_LIBRARIES})
  target_link_libraries(libcsp PRIVATE if_zmq)
endif()

if(HAVE_SYS_SOCKET_H)
  target_sources(libcsp PRIVATE csp_if_udp.c)
endif()
