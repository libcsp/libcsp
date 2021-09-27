target_sources(libcsp PRIVATE
  csp_bridge.c
  csp_buffer.c
  csp_conn.c
  csp_crc32.c
  csp_debug.c
  csp_dedup.c
  csp_endian.c
  csp_hex_dump.c
  csp_id.c
  csp_iflist.c
  csp_init.c
  csp_io.c
  csp_port.c
  csp_promisc.c
  csp_qfifo.c
  csp_route.c
  csp_service_handler.c
  csp_services.c
  )

add_subdirectory(arch)
add_subdirectory(crypto)
add_subdirectory(drivers)
add_subdirectory(interfaces)
add_subdirectory(rtable)
add_subdirectory(transport)