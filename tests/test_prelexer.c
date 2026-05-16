#include "prelexer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_root(void)
{
    pretoken_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/");

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_END);
    assert(tok.length == 0);
}

static void test_simple_path(void)
{
    pretoken_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/users/profile");

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 5);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 7);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_END);
    assert(tok.length == 0);
}

static void test_param_path(void)
{
    pretoken_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/users/profile/:user_id/edit");

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 5);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 7);

    tok = prelexer_next(&lx);
    assert(tok.length == 7);
    assert(tok.type == TOKEN_PARAM);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 4);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_END);
    assert(tok.length == 0);
}

static void test_trailing(void)
{
    pretoken_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/users/");

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 5);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_END);
    assert(tok.length == 0);
}

static void test_double_slash(void)
{
    pretoken_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "//");

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_ILLEGAL);
    assert(tok.length == 0);
}

static void test_param_brace(void)
{
    pretoken_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_BRACE;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/accounts/{account_id}");

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 8);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_PARAM);
    assert(tok.length == 10);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_END);
    assert(tok.length == 0);
}

static void test_param_angle(void)
{
    pretoken_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_ANGLE;

    prelexer_t lx = { 0 };
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/accounts/<account_id>");

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 8);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_PARAM);
    assert(tok.length == 10);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_END);
    assert(tok.length == 0);
}

int main(void)
{
    test_root();
    test_simple_path();
    test_param_path();
    test_trailing();
    test_double_slash();
    test_param_brace();
    test_param_angle();
    return 0;
}
