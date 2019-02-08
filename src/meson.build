deap_sources = [
  'main.c',
  'deap-application.c',
  'deap-window.c',
  'deap-gnome-shell.c',
  'deap-login1.c',

  'interfaces/deap-registrable.c',
]

deap_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk+-3.0', version: '>= 3.22'),
  dependency('libdazzle-1.0', version: '>= 3.31.4')
]

gnome = import('gnome')

deap_sources += gnome.compile_resources('deap-resources',
  'deap.gresource.xml',
  c_name: 'deap'
)

executable('deap', deap_sources,
  dependencies: deap_deps,
  install: true,
)