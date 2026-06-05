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
    // Null.
    if (request == NULL || length == 0)
        goto failure;

    // Enforce all requests begin with '/'.
    if (*request != '/')
        goto failure;

    // Guard against ridiculous sizes.
    if (length > LEXER_CHAR_LIMIT)
        goto failure;

    // Load.
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

    // Null.
    if (c == NULL || lx->length == 0)
        return tok; // Illegal.

    // End.
    if (c == end) {
        tok.type = TOKEN_END;
        return tok;
    }

    // Check for root '/' or trailing-slash.
    if (*c == '/' && c == end - 1) {
        tok.type = c == lx->str ? TOKEN_END : TOKEN_TRAILING;
        return tok;
    }

    // Check for double-slash.
    if (++c < end && *c == '/')
        return tok; // Illegal.

    // Consume until next '/'.
    for (start = c; c < end && *c != '/'; c++)
        if (*c == '*' || *c == '#' || *c == '?' || isspace(*c) || iscntrl(*c))
            return tok; // Illegal character.

    // Literal.
    lx->cursor = c;
    tok.type = TOKEN_LITERAL;
    tok.ptr = start;
    tok.length = (uint16_t)(c - start);
    return tok;
}
