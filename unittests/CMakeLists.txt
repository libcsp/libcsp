if(CHECK_FOUND)
  add_executable(csp_tests)
  target_link_libraries(csp_tests PRIVATE csp ${CHECK_LIBRARIES})
  target_sources(csp_tests PRIVATE
    main.c
    queue.c
    buffer.c
    hmac.c
  )
endif()
