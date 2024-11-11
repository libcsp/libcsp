add_executable(simple-send-usart EXCLUDE_FROM_ALL src/main.c)
target_include_directories(simple-send-usart PRIVATE ${csp_inc})
target_link_libraries(simple-send-usart PRIVATE csp)
