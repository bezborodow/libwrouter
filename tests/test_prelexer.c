#include "prelexer.h"
#include "token.h"
#include "helpers/token_helpers.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_root(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
}

static void test_empty(void)
{
    token_t tok;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, WROUTER_SYNTAX_COLON);

    prelexer_load(&lx, "");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
}

static void test_missing_leading_slash(void)
{
    token_t tok;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, WROUTER_SYNTAX_COLON);

    prelexer_load(&lx, "we/like/consistency");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
}

static void test_simple_path(void)
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
}

static void test_param_path(void)
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
}

static void test_trailing(void)
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
}

static void test_double_slash(void)
{
    token_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "//");

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
}

static void test_param_brace(void)
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
}

static void test_param_angle(void)
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
}

static void test_wildcard(void)
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

    tok = prelexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
}

int main(void)
{
    test_root();
    test_empty();
    test_missing_leading_slash();
    test_simple_path();
    test_param_path();
    test_trailing();
    test_double_slash();
    test_param_brace();
    test_param_angle();
    test_wildcard();
    return 0;
}
