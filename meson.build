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

inc = include_directories('include')

src = files(
    'src/arena.c',
    'src/builder.c',
    'src/dispatcher.c',
    'src/lexer.c',
    'src/prelexer.c',
    'src/router.c',
    'src/symbol.c',
)

libwrouter = library(
    'wrouter',
    src,
    include_directories : include_directories('include', 'src'),
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
    description : 'High-performance Uniform Resource Locator (URL) symbolic router library for the Web.',
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

foreach t : [
    'arena',
    'builder',
    'lexer',
    'prelexer',
    'router',
    'symbol',
]
    exe = executable(
        'test_@0@'.format(t),
        'tests/test_@0@.c'.format(t),
        src,
        include_directories : include_directories('include', 'src'),
        c_args : san_args,
        link_args : san_args,
    )

    test(t, exe)
endforeach

microhttpd_dep = dependency('libmicrohttpd', required: false)

if microhttpd_dep.found()
  subdir('examples/libmicrohttpd')
endif

py = import('python')
python = py.find_installation('python3', required: false)

if python.found()
  subdir('bindings/python')
endif
