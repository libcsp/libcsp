add_executable(csp_server_client EXCLUDE_FROM_ALL csp_server_client.c)
target_include_directories(csp_server_client PRIVATE ${csp_inc})
target_link_libraries(csp_server_client PRIVATE libcsp)
target_compile_definitions(csp_server_client PRIVATE NDEBUG)

add_executable(csp_arch EXCLUDE_FROM_ALL csp_arch.c)
target_include_directories(csp_arch PRIVATE ${csp_inc})
target_link_libraries(csp_arch PRIVATE libcsp)
target_compile_definitions(csp_arch PRIVATE NDEBUG)

add_executable(zmqproxy EXCLUDE_FROM_ALL zmqproxy.c)
target_include_directories(zmqproxy PRIVATE ${csp_inc})
target_link_libraries(zmqproxy PRIVATE libcsp)
target_compile_definitions(zmqproxy PRIVATE NDEBUG)
