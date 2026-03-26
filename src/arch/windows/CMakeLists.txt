target_sources(csp_es PRIVATE
  csp_clock.c
  csp_queue.c
  csp_semaphore.c
  csp_system.c
  csp_time.c
  csp_rand.c
  )

target_include_directories(csp_es PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)
target_link_libraries(csp_es PRIVATE Threads::Threads)
