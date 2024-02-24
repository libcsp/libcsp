target_sources(csp PRIVATE
  csp_bridge.c
  csp_buffer.c
  csp_conn.c
  csp_crc32.c
  csp_debug.c
  csp_dedup.c
  csp_id.c
  csp_iflist.c
  csp_init.c
  csp_io.c
  csp_port.c
  csp_qfifo.c
  csp_route.c
  csp_rtable_cidr.c
  csp_service_handler.c
  csp_services.c
  csp_rdp.c
  csp_rdp_queue.c
  csp_sfp.c
  )

set_source_files_properties(csp_rdp.c
  TARGET_DIRECTORY csp
  PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE
)

if (CSP_HAVE_STDIO)
  target_sources(csp PRIVATE csp_rtable_stdio.c)
endif()

if (CSP_ENABLE_CSP_PRINT)
  target_sources(csp PRIVATE csp_hex_dump.c)
endif()

if (CSP_USE_PROMISC)
  target_sources(csp PRIVATE csp_promisc.c)
endif()

add_subdirectory(arch)
add_subdirectory(crypto)
add_subdirectory(drivers)
add_subdirectory(interfaces)
