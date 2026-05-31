py = import('python')
python = py.find_installation('python3', required: true)

python_dep = python.dependency()

inc = include_directories('../../include')

py_install = python.get_install_dir()

wrouter_py = shared_module(
  'wrouter',
  [
    'builder.c',
    'dispatcher.c',
    'params.c',
    'router.c',
    'wroutermodule.c',
  ],
  name_prefix : '',
  include_directories : inc,
  dependencies : python_dep,
  link_with : libwrouter,
  install : true,
  install_dir : py_install,
)
