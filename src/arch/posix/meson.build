csp_sources += files([
	'csp_queue.c',
	'csp_semaphore.c',
	'csp_system.c',
	'csp_time.c',
	'csp_clock.c',
	'pthread_queue.c'
])

if conf.get('HAVE_ENDIAN_H', 0) == 0
	csp_sources += files('../std/csp_endian.c')
endif

csp_deps += dependency('threads')
