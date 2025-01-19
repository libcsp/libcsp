add_executable(simple-send-eth ${CSP_SAMPLES_EXCLUDE} src/main.c)
target_include_directories(simple-send-eth PRIVATE ${csp_inc})
target_link_libraries(simple-send-eth PRIVATE csp)
