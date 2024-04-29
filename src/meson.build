csp_sources += files([
	'csp_bridge.c',
	'csp_buffer.c',
	'csp_conn.c',
	'csp_crc32.c',
	'csp_debug.c',
	'csp_dedup.c',
	'csp_id.c',
	'csp_iflist.c',
	'csp_init.c',
	'csp_io.c',
	'csp_port.c',
	'csp_qfifo.c',
	'csp_route.c',
	'csp_service_handler.c',
	'csp_services.c',
	'csp_sfp.c',
])

if yaml_dep.found()
	csp_sources += files('csp_yaml.c')
endif

if get_option('use_rtable')
	csp_sources += files('csp_rtable_cidr.c')
	if get_option('have_stdio')
		csp_sources += files('csp_rtable_stdio.c')
	endif
endif

if get_option('enable_csp_print')
	csp_sources += files('csp_hex_dump.c')
endif

if get_option('use_promisc')
	csp_sources += files('csp_promisc.c')
endif

if get_option('use_rdp')
	csp_sources += files(
		'csp_rdp.c',
		'csp_rdp_queue.c',
	)
endif

subdir('arch')
subdir('crypto')
subdir('drivers')
subdir('interfaces')
