#include "lexer.h"
#include "helpers/token_helpers.h"
#include "token.h"
#include "common.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static void test_lexer_null(void)
{
    token_t tok;

    lexer_t lx = { 0 };
    lexer_load(&lx, NULL, 200);

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_lexer_root(void)
{
    token_t tok;
    char path[] = "/";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_END);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);
}

static void test_lexer_illegal_double_slash(void)
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
        assert(tok.ptr == NULL);
    }
}

static void test_lexer_illegal_empty(void)
{
    token_t tok;
    char path[] = "";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    // Should never advance beyond ILLEGAL, even if called repeatedly.
    for (int i = 0; i < 5; i++) {
        tok = lexer_next(&lx);
        ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
        assert(tok.length == 0);
        assert(tok.ptr == NULL);
    }
}

static void test_lexer_illegal_missing_leading_slash(void)
{
    token_t tok;
    char path[] = "missing/leading/slash";

    lexer_t lx = { 0 };
    lexer_load(&lx, path, strlen(path));

    // Should never advance beyond ILLEGAL, even if called repeatedly.
    for (int i = 0; i < 5; i++) {
        tok = lexer_next(&lx);
        ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
        assert(tok.length == 0);
        assert(tok.ptr == NULL);
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
    assert(tok.ptr == NULL);
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
    assert(tok.ptr == NULL);
}

static void test_lexer_stupid_long(void)
{
    token_t tok;
    lexer_t lx = { 0 };

    // Test a very loooong string.
    size_t len = LEXER_CHAR_LIMIT + 2;
    char *long_str = malloc(len + 1);
    memset(long_str, 'a', len);
    long_str[0] = '/';

    lexer_load(&lx, long_str, len);

    tok = lexer_next(&lx);
    ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
    assert(tok.length == 0);
    assert(tok.ptr == NULL);

    free(long_str);
}

static void test_lexer_other_illegals(void)
{
    const char *cases[] = {
        "", "/?", "/#", "/*", "/ ", "/\t", "/\n", "/\r", "/\x01", "/\x7f", NULL
    };

    for (size_t i = 0; cases[i] != NULL; i++) {
        token_t tok;

        lexer_t lx = { 0 };
        lexer_load(&lx, cases[i], strlen(cases[i]));

        tok = lexer_next(&lx);

        ASSERT_TOKEN_TYPE(tok, TOKEN_ILLEGAL);
        assert(tok.length == 0);
        assert(tok.ptr == NULL);
    }
}

int main(void)
{
    test_lexer_null();
    test_lexer_root();
    test_lexer_illegal_double_slash();
    test_lexer_illegal_empty();
    test_lexer_illegal_missing_leading_slash();
    test_lexer_easy();
    test_lexer_trailing();
    test_lexer_stupid_long();
    test_lexer_other_illegals();

    return 0;
}
