#include "lexer.h"
#include "common.h"
#include "token.h"
#include "symbol.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void lexer_load(lexer_t *lx, const char *request, size_t length)
{
    if (request == NULL)
        goto failure;

    if (length == 0)
        goto failure;

    // Enforce all requests begin with '/'.
    if (*request != '/')
        goto failure;

    // Guard against ridiculous sizes.
    if (length > LEXER_CHAR_LIMIT)
        goto failure;

    lx->str = request;
    lx->cursor = request;
    lx->length = length;

    return;

failure:
    memset(lx, 0, sizeof(*lx));
}

token_t lexer_next(lexer_t *lx)
{
    const char *start, *c = lx->cursor;
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
    for (start = c; c < end && *c != '/'; c++)
        if (*c == '*' || *c == '#' || *c == '?' || isspace(*c) || iscntrl(*c))
            return tok; // Illegal.

    // If the token has a length, it is a literal.
    if ((tok.length = (uint16_t)(c - start))) {
        lx->cursor = c;
        tok.ptr = start;
        tok.type = TOKEN_LITERAL;
        return tok;
    }

    // Otherwise, end.
    tok.type = TOKEN_END;
    return tok;
}
