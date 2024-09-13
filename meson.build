project('csp', 'c', version: '2.1', license: 'LGPL', meson_version : '>=0.53.2', default_options : [
	'c_std=gnu11',
	'optimization=s',
	'warning_level=2',
	'werror=true'])

cc = meson.get_compiler('c')

conf = configuration_data()

conf.set('CSP_QFIFO_LEN', get_option('qfifo_len'))
conf.set('CSP_PORT_MAX_BIND', get_option('port_max_bind'))
conf.set('CSP_CONN_RXQUEUE_LEN', get_option('conn_rxqueue_len'))
conf.set('CSP_CONN_MAX', get_option('conn_max'))
conf.set('CSP_BUFFER_SIZE', get_option('buffer_size'))
conf.set('CSP_BUFFER_COUNT', get_option('buffer_count'))
conf.set('CSP_PACKET_PADDING_BYTES', get_option('packet_padding_bytes'))
conf.set('CSP_RDP_MAX_WINDOW', get_option('rdp_max_window'))
conf.set('CSP_RTABLE_SIZE', get_option('rtable_size'))

conf.set10('CSP_REPRODUCIBLE_BUILDS', get_option('enable_reproducible_builds'))

conf.set10('CSP_USE_RDP', get_option('use_rdp'))
conf.set10('CSP_USE_HMAC', get_option('use_hmac'))
conf.set10('CSP_USE_PROMISC', get_option('use_promisc'))
conf.set10('CSP_HAVE_STDIO', get_option('have_stdio'))
conf.set10('CSP_ENABLE_CSP_PRINT', get_option('enable_csp_print'))
conf.set10('CSP_PRINT_STDIO', get_option('print_stdio'))
conf.set10('CSP_USE_RTABLE', get_option('use_rtable'))

csp_deps = []
csp_sources = []

# First try libyaml from dynamic library (os)
yaml_dep = dependency('yaml-0.1', required : false)

# Second try to include libyaml as a cmake subproject (shared library)
if yaml_dep.found() == false
	cmake = import('cmake')
	# This feature is too new for github CI meson version
	#yaml_opts = cmake.subproject_options()
	#yaml_opts.add_cmake_defines({'BUILD_SHARED_LIBS': true})
	#yaml_pro = cmake.subproject('yaml', required: false, options: yaml_opts)
	yaml_pro = cmake.subproject('yaml', required: false)
	if yaml_pro.found()
		yaml_dep = yaml_pro.dependency('yaml')
	endif
endif
csp_deps += yaml_dep
conf.set('CSP_HAVE_LIBYAML', yaml_dep.found())

# Libc dependency
clib = meson.get_compiler('c').find_library('c', required: false)
if not clib.found()
	clib = dependency('libc', fallback: ['picolibc', 'picolibc_dep'], required: true)
endif
csp_deps += clib

# Include paths
csp_inc = include_directories('include', 'src')

# Default compile options
csp_c_args = ['-Wshadow',
	      '-Wcast-align',
	      '-Wwrite-strings',
	      '-Wpointer-arith',
	      '-Wno-unused-parameter']

subdir('src')
subdir('include/csp')

if not meson.is_subproject()
  install_subdir('include', install_dir : '.')
  install_headers(csp_config_h, install_dir : 'include/csp')
endif


csp_lib = library('csp',
	sources: [csp_sources, csp_config_h],
	include_directories : csp_inc,
	dependencies : csp_deps,
	c_args : csp_c_args,
	install : false,
	pic:true,
)

# The following dependency variable is for parent projects to link
# against libcsp. https://mesonbuild.com/Subprojects.html
csp_dep = declare_dependency(
	include_directories : csp_inc,
	link_with : csp_lib,
	dependencies : csp_deps,
)

subdir('examples')

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
