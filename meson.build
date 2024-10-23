project('nota', 'c')

nota_inc = include_directories('.')

quickjs = dependency('quickjs', fallback:['quickjs', 'quickjs_dep'], static:true)

libtype = get_option('default_library')
if libtype == 'static'
  add_project_arguments('-DJS_SHARED_LIBRARY', language:'c')
endif

qjs_nota_lib = library('qjs-nota',
  'qjs_nota.c',
  include_directories: nota_inc,
  dependencies: [quickjs]
)

qjs_nota_dep = declare_dependency(
  link_with: qjs_nota_lib,
  include_directories : nota_inc
)

nota_dep = declare_dependency(
  include_directories: nota_inc
)
