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
    'src/lexer.c',
    'src/parser.c',
    'src/prelexer.c',
    'src/preparser.c',
    'src/router.c',
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
test_prelexer = executable(
    'test_prelexer',
    'tests/test_prelexer.c',
    src,
    include_directories : inc,
)

test(
    'prelexer',
    test_prelexer,
)

test_lexer = executable(
    'test_lexer',
    'tests/test_lexer.c',
    src,
    include_directories : inc,
)

test(
    'lexer',
    test_lexer,
)
