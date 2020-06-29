if get_option('with_rtable') == 'cidr'
	csp_sources += files('csp_rtable_cidr.c')
	csp_sources += files('csp_rtable.c')
elif get_option('with_rtable') == 'static'
	csp_sources += files('csp_rtable_static.c')
	csp_sources += files('csp_rtable.c')
else
  	error('with_rtable must be set to either "cidr or static"')
endif

