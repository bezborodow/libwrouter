project(
    'libwrouter',
    'c',
    version : '0.1.0',
    default_options : [
        'c_std=c11',
        'warning_level=3',
    ],
)

inc = include_directories(
    'include',
    'src',
)

src = files(
    'src/arena.c',
    'src/builder.c',
    'src/lexer.c',
    'src/parser.c',
    'src/prelexer.c',
    'src/preparser.c',
    'src/router.c',
    'src/symbol.c',
    'src/trie.c',
)

libwrouter = library(
    'wrouter',
    src,
    include_directories : inc,
    install : true,
)

install_headers(
    'include/wrouter.h',
)

pkg = import('pkgconfig')

pkg.generate(
    libwrouter,
    name : 'libwrouter',
    description : 'High-performance URL router library.',
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
    'symbol',
]
    exe = executable(
        'test_@0@'.format(t),
        'tests/test_@0@.c'.format(t),
        src,
        include_directories : inc,
        c_args : san_args,
        link_args : san_args,
    )

    test(t, exe)
endforeach
