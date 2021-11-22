
yaml_dep = dependency('yaml-0.1', required : false)
conf.set('CSP_HAVE_LIBYAML', yaml_dep.found())
if yaml_dep.found()
    csp_deps += yaml_dep
    csp_sources += files('iflist_yaml.c')
endif
