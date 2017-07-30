csp_sources += files([
	'csp_malloc.c',
	'csp_queue.c',
	'csp_semaphore.c',
	'csp_system.c',
	'csp_thread.c',
	'csp_time.c',
])

csp_deps += dependency('freertos', fallback : ['c21', 'freertos_headers_dep'], required: true)
