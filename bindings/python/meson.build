py = import('python')
python = py.find_installation('python3', required: true)

python_dep = python.dependency()
inc = include_directories('../../include')

wrouter_py = python.extension_module(
    'wrouter',
    [
        'builder.c',
        'dispatcher.c',
        'router.c',
        'wroutermodule.c',
    ],
    include_directories : inc,
    dependencies : [python_dep, libwrouter_static_dep],
    install : true,
)
