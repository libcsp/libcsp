target_sources(csp_es PRIVATE
  csp_if_lo.c
  csp_if_kiss.c
  csp_if_i2c.c
  csp_if_tun.c
  csp_if_can.c
  csp_if_can_pbuf.c
  csp_if_eth.c
  csp_if_eth_pbuf.c
  csp_if_rf.c
  )

if(LIBZMQ_FOUND)
  # Add ZMQ interface source directly to csp_es to avoid export issues
  target_sources(csp_es PRIVATE csp_if_zmqhub.c)
  target_include_directories(csp_es PRIVATE ${LIBZMQ_INCLUDE_DIRS})
  target_link_libraries(csp_es PRIVATE ${LIBZMQ_LIBRARIES})
endif()

if(HAVE_SYS_SOCKET_H AND HAVE_ARPA_INET_H)
  target_sources(csp_es PRIVATE csp_if_udp.c)
endif()
