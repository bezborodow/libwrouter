cpp_sources = files('wrouter.cpp')

cpp_include_dirs = include_directories('.')
c_include_dirs = include_directories('../../include')

# 1. First define the library
wrouter_cpp = library(
  'wrouter_cpp',
  cpp_sources,
  include_directories : [cpp_include_dirs, c_include_dirs],
  dependencies : [libwrouter_dep],
  install : true,
)

# 2. Then expose dependency
wrouter_cpp_dep = declare_dependency(
  link_with : wrouter_cpp,
  include_directories : [cpp_include_dirs, c_include_dirs],
  dependencies : [libwrouter_dep],
)

cpp_test = executable(
  'test_cpp',
  'tests/test_cpp.cpp',
  dependencies : wrouter_cpp_dep,
)

test('cpp', cpp_test)
