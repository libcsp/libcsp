csp_sources += files('csp_if_lo.c')
csp_sources += files(['csp_if_can.c', 'csp_if_can_pbuf.c'])
csp_sources += files('csp_if_kiss.c')

zmq_dep = dependency('libzmq', version: '>4.0', required: false)
if zmq_dep.found()
	csp_sources += files('csp_if_zmqhub.c')
	csp_deps += zmq_dep
endif

if meson.get_compiler('c').has_header('sys/socket.h')
	csp_sources += files('csp_if_udp.c')
endif