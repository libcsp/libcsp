if get_option('enable_driver_socketcan')
	csp_sources += files(['can/can_socketcan.c'])
endif
if get_option('enable_driver_serial')
	csp_sources += files(['usart/usart_@0@.c'.format(host_machine.system())])
endif


