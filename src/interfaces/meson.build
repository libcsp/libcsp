sources = ['csp_if_lo.c']
deps = []

if get_option('enable_if_can')
	sources += ['csp_if_can.c', 'csp_if_can_pbuf.c']
endif
if get_option('enable_if_i2c')
	sources += 'csp_if_i2c.c'
endif
if get_option('enable_if_kiss')
	sources += 'csp_if_kiss.c'
endif
if get_option('enable_if_zmqhub')
	sources += 'csp_if_zmqhub.c'
	deps += dependency('libzmq', version: '>4.0')
endif	
	
csp_deps += declare_dependency(sources: sources, dependencies: deps)