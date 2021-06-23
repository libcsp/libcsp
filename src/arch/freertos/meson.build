csp_sources += files([
	'csp_malloc.c',
	'csp_queue.c',
	'csp_semaphore.c',
	'csp_system.c',
	'csp_thread.c',
	'csp_time.c',
	'csp_clock.c',
])

# FreeRTOS can be provided by either a freertos subproject, or a bsp subproject
csp_deps += dependency('freertos', fallback : ['freertos', 'freertos_dep'], required: false)
csp_deps += dependency('freertos', fallback : ['bsp', 'bsp_dep'], required: false)
