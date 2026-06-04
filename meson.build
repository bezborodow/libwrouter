project(
    'libwrouter',
    'c',
    version : '0.1.0',
    default_options : [
        'c_std=c11',
        'warning_level=3',
    ],
)

add_project_arguments(
    '-Wpedantic',
    '-Wjump-misses-init',
    '-Wmissing-field-initializers',
    language : 'c'
)

python_package = get_option('python_package')
inc = include_directories('include')
src_inc = include_directories('include', 'src')

src = files(
    'src/arena.c',
    'src/builder.c',
    'src/compiler.c',
    'src/dispatcher.c',
    'src/errors.c',
    'src/graph.c',
    'src/lexer.c',
    'src/params.c',
    'src/prelexer.c',
    'src/router.c',
    'src/segment.c',
    'src/symbol.c',
    'src/terminal.c',
    'src/token.c',
)

libwrouter_static = static_library(
    'wrouter_static',
    src,
    include_directories : src_inc,
    build_by_default : false,
)

libwrouter_static_dep = declare_dependency(
    link_with : libwrouter_static,
    include_directories : inc,
)

# Python packaging builds should only emit the extension module and its
# bundled core objects. The full native install layout remains the default.
if not python_package
    libwrouter = library(
        'wrouter',
        src,
        include_directories : src_inc,
        install : true,
        version : meson.project_version(),
        soversion : '0',
    )

    libwrouter_dep = declare_dependency(
        link_with : libwrouter,
        include_directories : inc,
    )

    install_headers(
        'include/wrouter.h',
    )

    pkg = import('pkgconfig')

    pkg.generate(
        libwrouter,
        name : 'libwrouter',
        filebase : 'libwrouter',
        description : 'Symbolic Uniform Resource Locator (URL) router library for the Web.',
        version : meson.project_version(),
    )

    #
    # Unit tests
    #
    san_args = [
        '-fsanitize=address,undefined',
        '-fno-omit-frame-pointer',
        '-g',
    ]

    test_helpers = static_library(
        'test_helpers',
        'tests/helpers/token_helpers.c',
        include_directories : include_directories('include', 'src', 'tests'),
    )

    foreach t : [
        'arena',
        'builder',
        'dispatcher',
        'errors',
        'graph',
        'lexer',
        'params',
        'prelexer',
        'router',
        #'symbol',
        'terminal',
    ]
        exe = executable(
            'test_@0@'.format(t),
            'tests/test_@0@.c'.format(t),
            src,
            link_with : test_helpers,
            include_directories : include_directories('include', 'src', 'tests'),
            c_args : san_args,
            link_args : san_args,
        )

        test(t, exe)
    endforeach

    microhttpd_dep = dependency('libmicrohttpd', required: false)

    if microhttpd_dep.found()
        subdir('examples/libmicrohttpd')
    endif

    add_languages('cpp', required : true)
    subdir('bindings/cpp')
    subdir('examples/cpp')
endif

py = import('python')
python = py.find_installation('python3', required: false)

if python.found()
    subdir('bindings/python')
endif
