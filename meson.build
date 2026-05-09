project(
    'libwrouter',
    'c',
    version : '0.1.0',
    default_options : [
        'c_std=c11',
        'warning_level=3',
    ],
)

inc = include_directories('include')

src = files(
    'src/lexer.c',
    'src/parser.c',
    'src/trie.c',
    'src/router.c',
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
    description : 'High performance URL router library',
)
