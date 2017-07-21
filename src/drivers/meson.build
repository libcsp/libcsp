sources = []

if get_option('enable_driver_socketcan')
	sources += ['can/can_socketcan.c']
endif
if get_option('enable_driver_serial')
	sources += ['usart/usart_@0@.c'.format(host_machine.system())]
endif
	
csp_deps += declare_dependency(sources: sources)

