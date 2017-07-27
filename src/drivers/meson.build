cc = meson.get_compiler('c')

socketcan_dep = dependency('libsocketcan', required: false)
if socketcan_dep.found()
	conf.set('CSP_HAVE_LIBSOCKETCAN', true)
	csp_deps += socketcan_dep
endif

if cc.has_header('linux/socket.h')
	csp_sources += files(['can/can_socketcan.c'])
endif

if cc.has_header('termios.h') and cc.has_header('pthread.h')
	csp_sources += files(['usart/usart_@0@.c'.format(host_machine.system())])
endif


