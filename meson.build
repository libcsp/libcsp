project('libcsp', 'c', version: '1.5.0', license: 'LGPL')

cc = meson.get_compiler('c')

conf = configuration_data()

conf.set('CSP_CONN_MAX', 8)
conf.set('CSP_CONN_QUEUE_LENGTH', 5)
conf.set('CSP_FIFO_INPUT', 10)
conf.set('CSP_MAX_BIND_PORT', 16)
conf.set('CSP_RDP_MAX_WINDOW', 20)
conf.set('CSP_PADDING_BYTES', 8)
conf.set('CSP_CONNECTION_SO', 0)

conf.set('CSP_DEBUG', 1)
conf.set('CSP_LOG_LEVEL_DEBUG', 1)
conf.set('CSP_LOG_LEVEL_INFO', 1)
conf.set('CSP_LOG_LEVEL_WARN', 1)
conf.set('CSP_LOG_LEVEL_ERROR', 1)

conf.set('CSP_LITTLE_ENDIAN', 1)

conf.set('CSP_HAVE_STDBOOL_H', cc.has_header('stdbool.h'))

conf.set('CSP_USE_RDP', true)
conf.set('CSP_USE_CRC32', true)
conf.set('CSP_USE_HMAC', true)
conf.set('CSP_USE_XTEA', true)
conf.set('CSP_USE_PROMISC', false)
conf.set('CSP_USE_QOS', false)
conf.set('CSP_USE_DEDUP', false)
conf.set('CSP_USE_INIT_SHUTDOWN', false)

csp_sources = files([
	'src/crypto/csp_hmac.c',
	'src/crypto/csp_sha1.c',
	'src/crypto/csp_xtea.c',
	'src/transport/csp_rdp.c',
	'src/transport/csp_udp.c',
	'src/csp_buffer.c',
	'src/csp_bridge.c',
	'src/csp_conn.c',
	'src/csp_crc32.c',
	'src/csp_debug.c',
	'src/csp_dedup.c',
	'src/csp_endian.c',
	'src/csp_hex_dump.c',
	'src/csp_iflist.c',
	'src/csp_io.c',
	'src/csp_port.c',
	'src/csp_promisc.c',
	'src/csp_qfifo.c',
	'src/csp_route.c',
	'src/csp_service_handler.c',
	'src/csp_services.c',
	'src/csp_sfp.c',
])

csp_deps = []
subdir('src/rtable')
subdir('src/arch')
subdir('src/drivers')
subdir('src/interfaces')

csp_config_h = configure_file(output: 'csp_config.h', configuration: conf)

libcsp_inc = include_directories(['.', 'include'])

libcsp = static_library('csp',
	csp_sources,
	include_directories : libcsp_inc,
	dependencies : csp_deps,
	install: true
	)
	
libcsp_dep = declare_dependency(
	sources: csp_config_h,
	include_directories : libcsp_inc,
	dependencies : csp_deps,
	link_with: libcsp
	)
