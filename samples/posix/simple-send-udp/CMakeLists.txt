add_executable(simple-send-udp ${CSP_SAMPLES_EXCLUDE} src/main.c)
target_include_directories(simple-send-udp PRIVATE ${csp_inc})
target_link_libraries(simple-send-udp PRIVATE csp)
