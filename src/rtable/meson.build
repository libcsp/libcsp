if get_option('with_rtable') == 'cidr'
	sources = 'csp_rtable_cidr.c'
elif get_option('with_rtable') == 'static'
	sources = 'csp_rtable_static.c'
else
  	error('with_rtable must be set to either "cidr or static"')
endif

csp_deps += declare_dependency(sources: sources)
