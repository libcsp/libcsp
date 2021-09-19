socketcan_dep = dependency('libsocketcan', required: false)
if socketcan_dep.found()
	conf.set('CSP_HAVE_LIBSOCKETCAN', 1)
	csp_deps += socketcan_dep
else
	conf.set('CSP_HAVE_LIBSOCKETCAN', 0)
endif

if host_machine.system() == 'linux'
	csp_sources += files(['can/can_socketcan.c'])
	csp_sources += files(['usart/usart_linux.c'])
	csp_sources += files(['usart/usart_kiss.c'])
elif host_machine.system() == 'windows'
	csp_sources += files(['usart/usart_windows.c'])
	csp_sources += files(['usart/usart_kiss.c'])
endif
