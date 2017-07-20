sources = [
	'csp_malloc.c',
	'csp_queue.c',
	'csp_semaphore.c',
	'csp_system.c',
	'csp_thread.c',
	'csp_time.c',
	'pthread_queue.c'
]

thread_dep = dependency('threads')

csp_deps += declare_dependency(
	sources: sources,
	dependencies: thread_dep

)
