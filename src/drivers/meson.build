cc = meson.get_compiler('c')

socketcan_dep = dependency('libsocketcan', required: false)
if socketcan_dep.found()
	conf.set('CSP_HAVE_LIBSOCKETCAN', true)
	csp_deps += socketcan_dep
endif

if host_machine.system() == 'linux'
	csp_sources += files(['can/can_socketcan.c'])
	csp_sources += files(['usart/usart_linux.c'])
elif host_machine.system() == 'windows'
	csp_sources += files(['usart/usart_windows.c'])
endif
