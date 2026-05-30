python_dep = python.dependency()

inc = include_directories('../../include')

wrouter_py = shared_module(
  'wrouter',
  'wroutermodule.c',
  name_prefix : '',
  include_directories : inc,
  dependencies : [python_dep],
  link_with : libwrouter,
  install : true,
  install_dir : python.get_install_dir(),
)
