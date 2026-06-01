#include "lexer.h"
#include "token.h"
#include "symbol.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void lexer_load(lexer_t *lx, const char *request, size_t length)
{
    // Enforce all requests begin with '/'.
    if (*request != '/') {
        memset(lx, 0, sizeof(*lx));
        return;
    }

    lx->str = request;
    lx->cursor = request;
    lx->length = length;
}

token_t lexer_next(lexer_t *lx)
{
    const char *c = lx->cursor;
    const char *end = lx->str + lx->length;
    token_t tok = { 0 };

    if (c == NULL)
        return tok;

    // Check for root '/' or trailing-slash.
    if (*c == '/' && c == end - 1) {
        tok.type = c == lx->str ? TOKEN_END : TOKEN_TRAILING;
        return tok;
    }

    // Check for double-slash.
    if (++c < end && *c == '/') 
        return tok; // TOKEN_ILLEGAL.

    // Consume until next '/'.
    for (tok.ptr = c; c < end && *c != '/'; c++)
        ;

    // If the token has a length, it is a literal.
    if ((tok.length = (uint16_t)(c - tok.ptr))) {
        lx->cursor = c;
        tok.type = TOKEN_LITERAL;
        return tok;
    }

    // Otherwise, end.
    tok.type = TOKEN_END;
    return tok;
}
