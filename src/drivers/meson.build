socketcan_dep = dependency('libsocketcan', required: false)
if socketcan_dep.found()
	conf.set('CSP_HAVE_LIBSOCKETCAN', 1)
	csp_deps += socketcan_dep
	csp_sources += files(['can/can_socketcan.c'])
	if cc.has_function('can_set_canfd_bittiming', dependencies: socketcan_dep)
		conf.set('CSP_HAVE_LIBSOCKETCAN_CANFD', 1)
	else
		conf.set('CSP_HAVE_LIBSOCKETCAN_CANFD', 0)
	endif
else
	conf.set('CSP_HAVE_LIBSOCKETCAN', 0)
	conf.set('CSP_HAVE_LIBSOCKETCAN_CANFD', 0)
endif

if host_machine.system() == 'linux'
	csp_sources += files(['eth/eth_linux.c'])
	csp_sources += files(['usart/usart_linux.c'])
	csp_sources += files(['usart/usart_kiss.c'])
endif
