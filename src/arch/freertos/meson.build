csp_sources += files([
	'csp_queue.c',
	'csp_semaphore.c',
	'csp_system.c',
	'csp_time.c',
])

if conf.get('HAVE_ENDIAN_H', 0) == 0
	csp_sources += files('../std/csp_endian.c')
endif

# FreeRTOS can be provided by either a freertos subproject, or a bsp subproject
csp_deps += dependency('freertos', fallback : ['freertos', 'freertos_dep'], required: false)
