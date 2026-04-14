if(CHECK_FOUND)
  add_executable(csp_tests)
  target_link_libraries(csp_tests PRIVATE csp_es ${CHECK_LIBRARIES})
  target_sources(csp_tests PRIVATE
    main.c
    queue.c
    buffer.c
    hmac.c
    routing_enable_disable.c
  )

  # Register with CTest so `ctest` can discover and run the binary.
  # CK_XML_LOG_FILE_NAME tells libcheck to write its own XML alongside the
  # JUnit report that CTest produces via --output-junit.
  add_test(NAME csp_tests COMMAND csp_tests)
  set_tests_properties(csp_tests PROPERTIES
    ENVIRONMENT "CK_XML_LOG_FILE_NAME=${CMAKE_CURRENT_BINARY_DIR}/check_results.xml"
  )
endif()
