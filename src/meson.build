deap_sources = [
  'main.c',
  'deap-application.c',
  'deap-window.c',
  'deap-gnome-shell.c',
  'deap-login1.c',
  'deap-virtual-terminal.c',
]

deap_sources += [
  'logging/gtd-log.c',
]

includes = include_directories(
  '.',
  'logging',
)

deap_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk+-3.0', version: '>= 3.22'),
  dependency('libdazzle-1.0', version: '>= 3.31.4'),
  dependency('vte-2.91', version: '>= 0.56.0')
]

gnome = import('gnome')

deap_sources += gnome.compile_resources('deap-resources',
  'deap.gresource.xml',
  c_name: 'deap'
)

executable('deap',
  deap_sources,
  include_directories: includes,
  dependencies: deap_deps,
  install: true,
)
