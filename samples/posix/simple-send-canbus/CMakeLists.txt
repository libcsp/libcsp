add_executable(simple-send-canbus ${CSP_SAMPLES_EXCLUDE} src/main.c)
target_include_directories(simple-send-canbus PRIVATE ${csp_inc})
target_link_libraries(simple-send-canbus PRIVATE csp)
