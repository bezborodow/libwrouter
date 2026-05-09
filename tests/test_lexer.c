// tests/test_lexer.c

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

_Static_assert(sizeof(token_t) == 4, "token_t must be 4 bytes");

enum {
    SYMBOL_ACTION = 0,
    SYMBOL_SEGMENT,
    SYMBOL_USERS,
    SYMBOL_POSTS,
    SYMBOL_API,
    SYMBOL_V1,
    SYMBOL_ACCOUNT,
    SYMBOL_ACCOUNT_ID,
    SYMBOL_PROJECT,
    SYMBOL_EDIT,
    SYMBOL_STATIC,
    SYMBOL_A,
    SYMBOL_B,
    SYMBOL_C,
    SYMBOL_D,

    SYMBOL_ID = 0,
    SYMBOL_NAME,
    SYMBOL_PATH
};

typedef struct {
    const char *input;

    token_t expected[8];
    size_t expected_count;
} lexer_test_t;

static lexer_test_t tests[] = {
    {
        .input = "/",
        .expected_count = 0,
    },

    {
        .input = "/users",
        .expected = {
            { TOKEN_LITERAL, 5, SYMBOL_USERS },
        },
        .expected_count = 1,
    },

    {
        .input = "/users/",
        .expected = {
            { TOKEN_LITERAL, 5, SYMBOL_USERS },
        },
        .expected_count = 1,
    },

    {
        .input = "/users/posts",
        .expected = {
            { TOKEN_LITERAL, 5, SYMBOL_USERS },
            { TOKEN_LITERAL, 5, SYMBOL_POSTS },
        },
        .expected_count = 2,
    },

    {
        .input = "/users/:id",
        .expected = {
            { TOKEN_LITERAL, 5, SYMBOL_USERS },
            { TOKEN_PARAM,  2, SYMBOL_ID },
        },
        .expected_count = 2,
    },

    {
        .input = "/api/:name/posts",
        .expected = {
            { TOKEN_LITERAL, 3, SYMBOL_API },
            { TOKEN_PARAM,   4, SYMBOL_NAME },
            { TOKEN_LITERAL, 5, SYMBOL_POSTS },
        },
        .expected_count = 3,
    },

    {
        .input = "/static/*",
        .expected = {
            { TOKEN_LITERAL, 6, SYMBOL_STATIC },
            { TOKEN_WILDCARD, 1, 0 },
        },
        .expected_count = 2,
    },

    {
        .input = "/a/b/c/d",
        .expected = {
            { TOKEN_LITERAL, 1, SYMBOL_A },
            { TOKEN_LITERAL, 1, SYMBOL_B },
            { TOKEN_LITERAL, 1, SYMBOL_C },
            { TOKEN_LITERAL, 1, SYMBOL_D },
        },
        .expected_count = 4,
    },

    {
        .input = "/v1/users/:id/posts",
        .expected = {
            { TOKEN_LITERAL, 2, SYMBOL_V1 },
            { TOKEN_LITERAL, 5, SYMBOL_USERS },
            { TOKEN_PARAM,  2, SYMBOL_ID },
            { TOKEN_LITERAL, 5, SYMBOL_POSTS },
        },
        .expected_count = 4,
    },

    {
        .input = "/account/account_id/project/:id/edit",
        .expected = {
            { TOKEN_LITERAL, 7,  SYMBOL_ACCOUNT },
            { TOKEN_LITERAL, 10, SYMBOL_ACCOUNT_ID },
            { TOKEN_LITERAL, 7,  SYMBOL_PROJECT },
            { TOKEN_PARAM,   2,  SYMBOL_ID },
            { TOKEN_LITERAL, 4,  SYMBOL_EDIT },
        },
        .expected_count = 5,
    },
};

static void test_lexer(void)
{
    symbol_ctx_t symbols = {0};

    size_t test_count = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < test_count; i++) {

        lexer_t lx;

        lexer_init(&lx,
                   tests[i].input,
                   strlen(tests[i].input),
                   &symbols);

        for (size_t j = 0; j < tests[i].expected_count; j++) {

            token_t tok = lexer_next(&lx);
            token_t exp = tests[i].expected[j];

            assert(tok.type == exp.type);
            assert(tok.length == exp.length);
            assert(tok.symbol == exp.symbol);
        }

        token_t eof = lexer_next(&lx);

        assert(eof.type == TOKEN_EOF);
    }
}

int main(void)
{
    test_lexer();

    printf("Lexer tests passed.\n");

    return 0;
}
