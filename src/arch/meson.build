if host_machine.system() == 'linux'
	subdir('posix')
	conf.set('CSP_POSIX', 1)
elif host_machine.system() == 'freertos'
	subdir('freertos')
	conf.set('CSP_FREERTOS', 1)
else
	error('invalid system @0@'.format(host_machine.system()))
endif
