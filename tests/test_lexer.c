#include "lexer.h"
#include <assert.h>
#include <string.h>
#include "helpers/token_helpers.h"
#include "token.h"

static void test_lexer_root(void)
{
    token_t tok;
    char path[] = "/";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
}

static void test_lexer_illegal(void)
{
    token_t tok;
    char path[] = "//";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    // Should never advance beyond ILLEGAL, even if called repeatedly.
    for (int i = 0; i < 5; i++) {
        tok = lexer_next(&lx);
        ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
        assert(tok.length == 0);
    }
}

static void test_lexer_easy(void)
{
    token_t tok;
    char path[] = "/angle/euler/quaternion";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 5);
    assert(strncmp("angle", tok.ptr, 5) == 0);

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 5);
    assert(strncmp("euler", tok.ptr, 5) == 0);

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 10);
    assert(strncmp("quaternion", tok.ptr, 10) == 0);

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
}

static void test_lexer_trailing(void)
{
    token_t tok;
    char path[] = "/trailing/slash/";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 8);
    assert(strncmp("trailing", tok.ptr, 8) == 0);

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_LITERAL);
    assert(tok.length == 5);
    assert(strncmp("slash", tok.ptr, 5) == 0);

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_TRAILING);
    assert(tok.length == 0);
}

int main(void)
{
    test_lexer_root();
    test_lexer_illegal();
    test_lexer_easy();
    test_lexer_trailing();

    return 0;
}
