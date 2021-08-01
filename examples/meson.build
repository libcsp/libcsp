csp_c_args += '-DNDEBUG'

executable('csp_server_client',
	   'csp_server_client.c',
	   include_directories : csp_inc,
	   c_args : csp_c_args,
	   cpp_args : '-DNDEBUG',
	   link_with : csp_lib,
	   build_by_default : false)

executable('csp_arch',
	   'csp_arch.c',
	   include_directories : csp_inc,
	   c_args : csp_c_args,
	   cpp_args : '-DNDEBUG',
	   link_with : csp_lib,
	   build_by_default : false)

executable('zmqproxy',
	   'zmqproxy.c',
	   include_directories : csp_inc,
	   c_args : csp_c_args,
	   link_with : csp_lib,
	   build_by_default : false)
