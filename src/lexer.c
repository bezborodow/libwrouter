#include "lexer.h"
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

    token_t tok = { 0 };

    if (p >= end) {
        tok.type = TOKEN_END;
        return tok;
    }

    // Skip separators.
    if (*p == '/') {
        p++;
    }

    if (p >= end) {
        tok.type = TOKEN_END;
        return tok;
    }

    if (*p == '/') {
        tok.type = TOKEN_ILLEGAL;
        return tok;
    }

    for (tok.ptr = p; p < end && *p != '/'; p++)
        ;

    tok.length = p - tok.ptr;
    lx->cursor = p;

    if (tok.length) {
        tok.type = TOKEN_LITERAL;
        return tok;
    }

    tok.type = TOKEN_END;
    return tok;
}
