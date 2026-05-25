#include "lexer.h"
#include <assert.h>
#include <string.h>

static void test_lexer_illegal(void)
{
    token_t tok;
    char path[] = "//";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    tok = lexer_next(&lx);
    assert(tok.type == TOKEN_ILLEGAL);
    tok = lexer_next(&lx);
    assert(tok.type == TOKEN_ILLEGAL);
}

static void test_lexer_easy(void)
{
    token_t tok;
    char path[] = "/angle/euler/quaternion";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    tok = lexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 5);
    assert(strncmp("angle", tok.ptr, 5) == 0);

    tok = lexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 5);
    assert(strncmp("euler", tok.ptr, 5) == 0);

    tok = lexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 10);
    assert(strncmp("quaternion", tok.ptr, 10) == 0);

    tok = lexer_next(&lx);
    assert(tok.type == TOKEN_END);
    assert(tok.length == 0);
}

int main(void)
{
    test_lexer_easy();
    test_lexer_illegal();

    return 0;
}
