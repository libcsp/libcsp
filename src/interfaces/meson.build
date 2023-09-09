csp_sources += files('csp_if_lo.c')
csp_sources += files(['csp_if_can.c', 'csp_if_can_pbuf.c'])
csp_sources += files(['csp_if_eth.c', 'csp_if_eth_pbuf.c'])
csp_sources += files('csp_if_kiss.c')
csp_sources += files('csp_if_tun.c')
csp_sources += files('csp_if_i2c.c')

zmq_dep = dependency('libzmq', version: '>4.0', required: false)
if zmq_dep.found()
	conf.set('CSP_HAVE_LIBZMQ', 1)
	csp_sources += files('csp_if_zmqhub.c')
	csp_deps += zmq_dep
endif

if cc.has_header('sys/socket.h') and cc.has_header('arpa/inet.h')
	csp_sources += files('csp_if_udp.c')
endif
