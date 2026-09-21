socketcan_dep = dependency('libsocketcan', required: false)
if socketcan_dep.found()
	conf.set('CSP_HAVE_LIBSOCKETCAN', 1)
	csp_deps += socketcan_dep
	csp_sources += files(['can/can_socketcan.c'])
else
	conf.set('CSP_HAVE_LIBSOCKETCAN', 0)
endif

if host_machine.system() == 'linux'
	csp_sources += files(['eth/eth_linux.c'])
	if get_option('use_posix_termios')
		csp_sources += files(['usart/usart_posix.c'])
	else
		csp_sources += files(['usart/usart_linux.c'])
	endif
	csp_sources += files(['usart/usart_kiss.c'])
endif
