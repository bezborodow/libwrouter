#include "prelexer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_simple_path(void)
{
    pretoken_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = {0};
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/users/profile");
    
    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_END);
}

static void test_param_path(void)
{
    pretoken_t tok;

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_COLON;

    prelexer_t lx = {0};
    prelexer_init(&lx, param_syntax);

    prelexer_load(&lx, "/users/profile/:user_id/edit");
    
    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    
    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_PARAM);
    
    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);

    tok = prelexer_next(&lx);
    assert(tok.type == TOKEN_END);
}

int main(void)
{
    test_simple_path();
    test_param_path();
    return 0;
}
