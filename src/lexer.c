#include "lexer.h"
#include "token.h"
#include "symbol.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void lexer_load(lexer_t *lx, const char *request, size_t length)
{
    lx->str = request;
    lx->cursor = request;
    lx->length = length;
}

token_t lexer_next(lexer_t *lx)
{
    const char *p = lx->cursor;
    const char *end = lx->str + lx->length;

    if (p >= end)
        return make_token(TOKEN_END);

    // Skip separators.
    if (*p == '/')
        p++;

    if (p >= end)
        return make_token(TOKEN_END);

    if (*p == '/')
        return make_token(TOKEN_ILLEGAL);

    token_t tok = { 0 };
    tok.ptr = p;

    for (; p < end && *p != '/'; p++)
        ;

    tok.length = (uint16_t)(p - tok.ptr);
    lx->cursor = p;

    if (tok.length) {
        tok.type = TOKEN_LITERAL;
        return tok;
    }

    return make_token(TOKEN_END);
}
