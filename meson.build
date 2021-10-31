project('csp', 'c', version: '1.5.0', license: 'LGPL',
        meson_version : '>=0.53',
	default_options : ['c_std=gnu11',
			   'optimization=s',
			   'warning_level=2',
			   'werror=true'])

cc = meson.get_compiler('c')

conf = configuration_data()

conf.set('CSP_DEBUG', 1)

log_str = get_option('log-level')
if log_str == 'error'
  log_level = 1
elif log_str == 'warn'
  log_level = 2
elif log_str == 'info'
  log_level = 3
elif log_str == 'debug'
  log_level = 4
else
  log_level = 3
endif

conf.set10('CSP_LOG_LEVEL_ERROR', log_level >= 1)
conf.set10('CSP_LOG_LEVEL_WARN',  log_level >= 2)
conf.set10('CSP_LOG_LEVEL_INFO',  log_level >= 3)
conf.set10('CSP_LOG_LEVEL_DEBUG', log_level >= 4)

conf.set('CSP_QFIFO_LEN', get_option('qfifo_len'))
conf.set('CSP_PORT_MAX_BIND', get_option('port_max_bind'))
conf.set('CSP_CONN_RXQUEUE_LEN', get_option('conn_rxqueue_len'))
conf.set('CSP_CONN_MAX', get_option('conn_max'))
conf.set('CSP_BUFFER_SIZE', get_option('buffer_size'))
conf.set('CSP_BUFFER_COUNT', get_option('buffer_count'))
conf.set('CSP_RDP_MAX_WINDOW', get_option('rdp_max_window'))
conf.set('CSP_RTABLE_SIZE', get_option('rtable_size'))


conf.set10('CSP_USE_RDP', get_option('use_rdp'))
conf.set10('CSP_USE_CRC32', get_option('use_crc32'))
conf.set10('CSP_USE_HMAC', get_option('use_hmac'))
conf.set10('CSP_USE_XTEA', get_option('use_xtea'))
conf.set10('CSP_USE_PROMISC', get_option('use_promisc'))
conf.set10('CSP_USE_DEDUP', get_option('use_dedup'))

zmq_dep = dependency('libzmq', version: '>4.0', required: false)
if zmq_dep.found()
	conf.set('CSP_HAVE_LIBZMQ', 1)
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
	'src/csp_sfp.c',
])

csp_deps = []
subdir('src/rtable')
subdir('src/arch')
subdir('src/drivers')
subdir('src/interfaces')

csp_config_h = configure_file(output: 'csp_autoconfig.h', configuration: conf, install_dir: 'csp/')

csp_inc = include_directories(['.', 'include'])
if not meson.is_subproject()
  install_subdir('include', install_dir : '.')
  install_headers(csp_config_h, install_dir : 'include/csp')
endif

csp_c_args = ['-Wshadow',
	      '-Wcast-align',
	      '-Wwrite-strings',
	      '-Wno-unused-parameter']

clib = meson.get_compiler('c').find_library('c', required: false)
if not clib.found()
	clib = dependency('libc', fallback: ['picolibc', 'picolibc_dep'], required: true)
endif

csp_deps += clib

csp_lib = static_library('csp',
	sources: [csp_sources, csp_config_h],
	include_directories : csp_inc,
	dependencies : csp_deps,
	c_args : csp_c_args,
)

csp_shlib = shared_library('csp',
	sources: [csp_sources, csp_config_h],
	include_directories : csp_inc,
	dependencies : csp_deps,
	c_args : csp_c_args,
	install : true,
)

subdir('examples')

# The following dependency variable is for parent projects to link
# against libcsp. https://mesonbuild.com/Subprojects.html
csp_dep = declare_dependency(
	include_directories : csp_inc,
	link_with : csp_lib,
)

if get_option('enable_python3_bindings')
  py = import('python').find_installation('python3')
  # py.dependency() doesn't work with version constraint. Use plain
  # dependency() instead
  pydep = dependency('python3', version : '>=3.5', required : true)
  py.extension_module('libcsp_py3', 'src/bindings/python/pycsp.c',
                      c_args : csp_c_args,
                      dependencies : [csp_dep, pydep],
                      install : true)
endif
