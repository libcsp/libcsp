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
  csp_service_handler.c
  csp_services.c
  csp_sfp.c
  )

set_source_files_properties(csp_rdp.c
  TARGET_DIRECTORY csp
  PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE
)

if (CSP_USE_RTABLE)
  target_sources(csp PRIVATE csp_rtable_cidr.c)
  if (CSP_HAVE_STDIO)
    target_sources(csp PRIVATE csp_rtable_stdio.c)
  endif()
endif()

if (CSP_ENABLE_CSP_PRINT)
  target_sources(csp PRIVATE csp_hex_dump.c)
endif()

if (CSP_USE_PROMISC)
  target_sources(csp PRIVATE csp_promisc.c)
endif()

if (CSP_USE_RDP)
  target_sources(csp PRIVATE
    csp_rdp.c
    csp_rdp_queue.c
  )
endif()

if(LIBYAML_FOUND)
  add_library(csp_yaml OBJECT csp_yaml.c)
  target_include_directories(csp_yaml PRIVATE
    ${csp_inc}
    ${LIBYAML_INCLUDE_DIRS})
  target_link_libraries(csp_yaml PRIVATE csp_common ${LIBYAML_LIBRARIES})
  target_link_libraries(csp PRIVATE csp_yaml)
  if(BUILD_SHARED_LIBS)
    set_property(TARGET csp_yaml PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

add_subdirectory(arch)
add_subdirectory(crypto)
add_subdirectory(drivers)
add_subdirectory(interfaces)
