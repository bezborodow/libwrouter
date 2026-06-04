#include "common.h"
#include "prelexer.h"
#include "token.h"
#include "helpers/token_helpers.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void test_prelexer_null(void)
{
    token_t tok;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, WROUTER_SYNTAX_COLON);
    prelexer_load(&lx, NULL);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_root(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_empty(void)
{
    token_t tok;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, WROUTER_SYNTAX_COLON);

    prelexer_load(&lx, "");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_missing_leading_slash(void)
{
    token_t tok;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, WROUTER_SYNTAX_COLON);

    prelexer_load(&lx, "we/like/consistency");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_simple_path(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/users/profile");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 5);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "users", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 7);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "profile", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_param_path(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/users/profile/:user_id/edit");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 5);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "users", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 7);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "profile", tok.length) == 0);

    tok = prelexer_next(&lx);
    assert(tok.length == 7);
    ASSERT_TOKEN_TYPE(tok, TOKEN_PARAM);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "user_id", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 4);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "edit", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_trailing(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/users/");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 5);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "users", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_TRAILING);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_double_slash(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "//");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_param_brace(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_BRACE;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/accounts/{account_id}");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 8);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "accounts", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_PARAM);
    assert(tok.length == 10);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "account_id", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_param_angle(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_ANGLE;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/accounts/<account_id>");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 8);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "accounts", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_PARAM);
    assert(tok.length == 10);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "account_id", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_wildcard(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/downloads/*");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 9);
    assert(tok.ptr != NULL);
    assert(memcmp(tok.ptr, "downloads", tok.length) == 0);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_WILDCARD);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_prelexer_other_illegals(void)
{
    typedef struct {
        const char *input;
    } test_case_t;

    static const test_case_t cases[] = {
        { "" },
        { " " },
        { "//" },
        { "foo" },
        { "foo?" },
        { "bar*" },
        { ":foo" },
        { "/foo?" },
        { "/bar*" },
        { "/bar*bar" },
        { "/*bar" },
        { "/**" },
        { "/foo#" },
        { "/foo#bar" },
        { "/foo bar" },
        //{ "/foo%" }, TODO
        //{ "/foo%1" },
        { "/:foo:" },
        { "/:_foo" },
        { "/:foo$" },
        { "/:f:oo" },
        { "/:*" },
        { "/:1" },
        { "/:$" },
        { "/\n" },
        { "/\t" },
        { "/ " },
        { "/\x01" },
        { "/\x7f" },
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        prelexer_t lx = { 0 };
        prelexer_init(&lx, WROUTER_SYNTAX_COLON);
        prelexer_load(&lx, cases[i].input);

        token_t tok = prelexer_next(&lx);
        ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
        assert(tok.length == 0);
        assert(tok.ptr == NULL);
    }
}

static void test_prelexer_stupid(void)
{
    token_t tok;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, WROUTER_SYNTAX_COLON);

    // Test a very loooong string.
    size_t len = LEXER_CHAR_LIMIT + 2;

    char *long_str = malloc(len + 1);
    memset(long_str, 'a', len);
    long_str[len] = '\0';
    long_str[0] = '/';

    prelexer_load(&lx, long_str);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);

    free(long_str);
}

static void test_prelexer_uint16_length_overflow(void)
{
    token_t tok;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, WROUTER_SYNTAX_COLON);

    // The segment length must be rejected before it narrows into token_t.length.
    size_t len = (size_t)UINT16_MAX + 2;

    char *long_str = malloc(len + 1);
    assert(long_str != NULL);
    memset(long_str, 'a', len);
    long_str[len] = '\0';
    long_str[0] = '/';

    prelexer_load(&lx, long_str);

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);

    free(long_str);
}

int main(void)
{
    test_prelexer_null();
    test_prelexer_root();
    test_prelexer_empty();
    test_prelexer_missing_leading_slash();
    test_prelexer_simple_path();
    test_prelexer_param_path();
    test_prelexer_trailing();
    test_prelexer_double_slash();
    test_prelexer_param_brace();
    test_prelexer_param_angle();
    test_prelexer_wildcard();
    test_prelexer_other_illegals();
    test_prelexer_stupid();
    test_prelexer_uint16_length_overflow();
    return 0;
}
