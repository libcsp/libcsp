executable('csp_server_client',
	['csp_server_client.c', 'csp_server_client_posix.c'],
	include_directories : csp_inc,
	c_args : csp_c_args,
	dependencies : csp_dep,
	build_by_default : false)

executable('csp_arch',
	'csp_arch.c',
	include_directories : csp_inc,
	c_args : csp_c_args,
	dependencies :  csp_dep,
	build_by_default : false)

executable('zmqproxy',
	'zmqproxy.c',
	include_directories : csp_inc,
	c_args : csp_c_args,
	dependencies : csp_dep,
	build_by_default : false)
