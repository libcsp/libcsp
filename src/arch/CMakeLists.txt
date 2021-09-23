if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CSP_POSIX 1)
  add_subdirectory(posix)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CSP_WINDOWS 1)
  add_subdirectory(windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CSP_MACOSX 1)
  add_subdirectory(macosx)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeRTOS")
  set(CSP_FREERTOS 1)
  add_subdirectory(freertos)
else()
  message(FATAL_ERROR "invalid system ${CMAKE_SYSTEM_NAME}")
endif()

target_sources(libcsp PRIVATE
  csp_system.c
  csp_time.c
  )
