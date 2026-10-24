if(CHECK_FOUND)
  add_executable(csp_tests)
  target_link_libraries(csp_tests PRIVATE csp ${CHECK_LIBRARIES})
  target_sources(csp_tests PRIVATE
    main.c
    queue.c
    buffer.c
    hmac.c
    route.c
    dedup.c
    wire.c
  )
  if(HAVE_SYS_SOCKET_H AND HAVE_ARPA_INET_H)
    target_sources(csp_tests PRIVATE udp.c)
    target_compile_definitions(csp_tests PRIVATE CSP_TEST_UDP=1)
  endif()
  if(LIBZMQ_FOUND)
    target_sources(csp_tests PRIVATE zmqhub.c)
    target_include_directories(csp_tests PRIVATE ${LIBZMQ_INCLUDE_DIRS})
    target_link_libraries(csp_tests PRIVATE ${LIBZMQ_LIBRARIES})
  endif()
endif()
