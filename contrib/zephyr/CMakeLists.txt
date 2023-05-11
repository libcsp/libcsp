if(CONFIG_LIBCSP)
  set(CMAKE_SYSTEM_NAME "Zephyr")
  set(CMAKE_BUILD_TYPE None)

  # check_include_file() doesn't work in Zephyr module
  # https://github.com/zephyrproject-rtos/zephyr/issues/31193
  # predefine it
  set(HAVE_SYS_SOCKET_H OFF)

  add_subdirectory(../.. build)

  # Zephyr compiler options
  target_include_directories(csp PRIVATE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>
    )

  target_include_directories(csp SYSTEM PRIVATE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
    )

  target_compile_definitions(csp PRIVATE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>
    )

  target_compile_options(csp PRIVATE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>
    -fno-builtin -Wno-shadow -Wno-cast-align
    )

  add_dependencies(csp ${SYSCALL_LIST_H_TARGET})
  zephyr_append_cmake_library(csp)
  target_link_libraries(csp PUBLIC zephyr_interface)
endif()
