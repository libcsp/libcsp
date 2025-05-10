target_sources(csp PRIVATE
  csp_clock.c
  csp_queue.c
  csp_semaphore.c
  csp_system.c
  csp_time.c
  pthread_queue.c
  )

target_link_libraries(csp)
