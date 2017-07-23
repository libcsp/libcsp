csp_sources += files('csp_if_lo.c')

if get_option('enable_if_can')
	csp_sources += files(['csp_if_can.c', 'csp_if_can_pbuf.c'])
endif
if get_option('enable_if_i2c')
	csp_sources += files('csp_if_i2c.c')
endif
if get_option('enable_if_kiss')
	csp_sources += files('csp_if_kiss.c')
endif
if get_option('enable_if_zmqhub')
	csp_sources += files('csp_if_zmqhub.c')
	csp_deps += dependency('libzmq', version: '>4.0')
endif	
