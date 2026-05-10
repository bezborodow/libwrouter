#include "lexer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(token_t) == 4, "token_t must be 4 bytes");

typedef enum {
    LITERAL_A = 0,
    LITERAL_ACCOUNT,
    LITERAL_API,
    LITERAL_B,
    LITERAL_C,
    LITERAL_D,
    LITERAL_EDIT,
    LITERAL_POSTS,
    LITERAL_PROJECT,
    LITERAL_STATIC,
    LITERAL_USERS,
    LITERAL_V1,
    LITERAL_COUNT
} literal_id_t;

static const char *literal_table[] = { "a",    "account", "api",     "b",      "c",     "d",
                                       "edit", "posts",   "project", "static", "users", "v1" };

typedef enum { PARAM_ACCOUNT_ID = 0, PARAM_ID, PARAM_NAME, PARAM_COUNT } param_id_t;

static const char *param_table[] = { "account_id", "id", "name" };

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
        .input = "/invalid-symbol",
        .expected = {
            { TOKEN_ILLEGAL, 15, 0 },
        },
        .expected_count = 0,
    },

    /*

    {
        .input = "/users",
        .expected = {
            { TOKEN_LITERAL, 5, LITERAL_USERS },
        },
        .expected_count = 1,
    },

    {
        .input = "/users/",
        .expected = {
            { TOKEN_LITERAL, 5, LITERAL_USERS },
        },
        .expected_count = 1,
    },

    {
        .input = "/users/posts",
        .expected = {
            { TOKEN_LITERAL, 5, LITERAL_USERS },
            { TOKEN_LITERAL, 5, LITERAL_POSTS },
        },
        .expected_count = 2,
    },

    {
        .input = "/users/:id",
        .expected = {
            { TOKEN_LITERAL, 5, LITERAL_USERS },
            { TOKEN_PARAM,   2, PARAM_ID },
        },
        .expected_count = 2,
    },

    {
        .input = "/api/:name/posts",
        .expected = {
            { TOKEN_LITERAL, 3, LITERAL_API },
            { TOKEN_PARAM,   4, PARAM_NAME },
            { TOKEN_LITERAL, 5, LITERAL_POSTS },
        },
        .expected_count = 3,
    },

    {
        .input = "/static/" "*",
        .expected = {
            { TOKEN_LITERAL,  6, LITERAL_STATIC },
            { TOKEN_WILDCARD, 1, 0 },
        },
        .expected_count = 2,
    },

    {
        .input = "/a/b/c/d",
        .expected = {
            { TOKEN_LITERAL, 1, LITERAL_A },
            { TOKEN_LITERAL, 1, LITERAL_B },
            { TOKEN_LITERAL, 1, LITERAL_C },
            { TOKEN_LITERAL, 1, LITERAL_D },
        },
        .expected_count = 4,
    },

    {
        .input = "/v1/users/:id/posts",
        .expected = {
            { TOKEN_LITERAL, 2, LITERAL_V1 },
            { TOKEN_LITERAL, 5, LITERAL_USERS },
            { TOKEN_PARAM,   2, PARAM_ID },
            { TOKEN_LITERAL, 5, LITERAL_POSTS },
        },
        .expected_count = 4,
    },

    {
        .input = "/account/:account_id/project/:id/edit",
        .expected = {
            { TOKEN_LITERAL, 7,  LITERAL_ACCOUNT },
            { TOKEN_PARAM,   10, PARAM_ACCOUNT_ID },
            { TOKEN_LITERAL, 7,  LITERAL_PROJECT },
            { TOKEN_PARAM,   2,  PARAM_ID },
            { TOKEN_LITERAL, 4,  LITERAL_EDIT },
        },
        .expected_count = 5,
    },
    */
};

static void test_lexer(void)
{
    size_t test_count = sizeof(tests) / sizeof(tests[0]);

    symbol_ctx_t syms = { 0 };
    syms.literals.base = literal_table;
    syms.literals.count = (uint16_t)(sizeof(literal_table) / sizeof(literal_table[0]));
    syms.params.base = param_table;
    syms.params.count = (uint16_t)(sizeof(param_table) / sizeof(param_table[0]));

    for (size_t i = 0; i < test_count; i++) {

        lexer_t lx = { 0 };
        lexer_init(&lx, tests[i].input, strlen(tests[i].input), &syms);

        for (size_t j = 0; j < tests[i].expected_count; j++) {

            token_t tok = lexer_next(&lx);
            token_t exp = tests[i].expected[j];

            assert(tok.type == exp.type);
            assert(tok.length == exp.length);
            assert(tok.symbol == exp.symbol);
        }

        token_t eof = lexer_next(&lx);

        assert(eof.type == TOKEN_END);
    }
}

int main(void)
{
    test_lexer();

    printf("Lexer tests passed.\n");

    return 0;
}
