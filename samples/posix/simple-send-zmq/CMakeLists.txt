add_executable(simple-send-zmq EXCLUDE_FROM_ALL src/main.c)
target_include_directories(simple-send-zmq PRIVATE ${csp_inc})
target_link_libraries(simple-send-zmq PRIVATE csp)
