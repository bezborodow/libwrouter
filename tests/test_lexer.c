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
    char path[] = "/a/b/c";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    tok = lexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 1);
    assert(strncmp("a", tok.ptr, 1) == 0);

    tok = lexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 1);
    assert(strncmp("b", tok.ptr, 1) == 0);

    tok = lexer_next(&lx);
    assert(tok.type == TOKEN_LITERAL);
    assert(tok.length == 1);
    assert(strncmp("c", tok.ptr, 1) == 0);

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
