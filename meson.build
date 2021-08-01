project('csp', 'c', version: '1.5.0', license: 'LGPL',
	default_options : ['c_std=gnu99',
			   'optimization=s',
			   'warning_level=2',
			   'werror=true'])

cc = meson.get_compiler('c')

conf = configuration_data()

conf.set('CSP_DEBUG', 1)
conf.set('CSP_LOG_LEVEL_DEBUG', 1)
conf.set('CSP_LOG_LEVEL_INFO', 1)
conf.set('CSP_LOG_LEVEL_WARN', 1)
conf.set('CSP_LOG_LEVEL_ERROR', 1)

conf.set('CSP_VERSION', get_option('version'))

conf.set10('CSP_USE_RDP', get_option('use_rdp'))
conf.set10('CSP_USE_CRC32', get_option('use_crc32'))
conf.set10('CSP_USE_HMAC', get_option('use_hmac'))
conf.set10('CSP_USE_XTEA', get_option('use_xtea'))
conf.set10('CSP_USE_PROMISC', get_option('use_promisc'))
conf.set10('CSP_USE_DEDUP', get_option('use_dedup'))

if host_machine.endian() == 'little'
	conf.set('CSP_LITTLE_ENDIAN', 1)
else
	conf.set('CSP_BIG_ENDIAN', 1)
endif

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
	'src/csp_init.c',
	'src/csp_io.c',
	'src/csp_port.c',
	'src/csp_promisc.c',
	'src/csp_qfifo.c',
	'src/csp_route.c',
	'src/csp_service_handler.c',
	'src/csp_services.c',
	'src/csp_id.c',
])

csp_deps = []
subdir('src/rtable')
subdir('src/arch')
subdir('src/drivers')
subdir('src/interfaces')

csp_config_h = configure_file(output: 'csp_autoconfig.h', configuration: conf, install: true, install_dir: 'csp/')

csp_inc = include_directories(['.', 'include'])
install_subdir('include', install_dir : '.')
install_headers(csp_config_h, install_dir : 'include/csp')

csp_c_args = ['-Wshadow',
	      '-Wcast-align',
	      '-Wwrite-strings',
	      '-Wno-unused-parameter']

csp_deps += dependency('freertos', required: false)

csp_lib = static_library('csp',
	sources: [csp_sources, csp_config_h],
	include_directories : csp_inc,
	dependencies : csp_deps,
	c_args : csp_c_args,
)

subdir('examples')

# The following dependency variable is for parent projects to link
# against libcsp. https://mesonbuild.com/Subprojects.html
csp_dep = declare_dependency(
	include_directories : csp_inc,
	link_with : csp_lib,
)
