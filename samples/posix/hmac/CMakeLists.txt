add_executable(sample-hmac EXCLUDE_FROM_ALL src/main.c)
target_include_directories(sample-hmac PRIVATE ${csp_inc})
target_link_libraries(sample-hmac PRIVATE csp)
