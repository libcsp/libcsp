add_executable(simple-sfp-send-recv ${CSP_SAMPLES_EXCLUDE} src/main.c)
target_include_directories(simple-sfp-send-recv PRIVATE ${csp_inc})
target_link_libraries(simple-sfp-send-recv PRIVATE csp Threads::Threads)
