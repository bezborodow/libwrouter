#include "prelexer.h"
#include "common.h"
#include "token.h"
#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void prelexer_init(prelexer_t *lx, wrouter_param_syntax_t param_syntax)
{
    lx->cursor = NULL;
    lx->param_syntax = param_syntax;
}

void prelexer_load(prelexer_t *lx, const char *pattern)
{
    if (pattern == NULL)
        goto failure;

    if (*pattern == '\0')
        goto failure;

    // Enforce all patterns begin with '/'.
    if (*pattern != '/')
        goto failure;

    lx->str = pattern;
    lx->cursor = pattern;

    return;

failure:
    memset(lx, 0, sizeof(*lx));
    return;
}

/**
 * Get next route pattern token.
 *
 * Valid parameter names are [A-Za-z][A-Za-z0-9_]*. E.g., ":param1", but not
 * ":_param1" or ":1param".
 */
token_t prelexer_next(prelexer_t *lx)
{
    token_t tok = { 0 };
    const char *c = lx->cursor;

    const bool angle = lx->param_syntax == WROUTER_SYNTAX_ANGLE;
    const bool brace = lx->param_syntax == WROUTER_SYNTAX_BRACE;
    const bool colon = lx->param_syntax == WROUTER_SYNTAX_COLON;
    size_t extra;
    size_t length;
    const char *start;

    if (lx->cursor == NULL)
        goto finish;

    if (*c == '\0') {
        tok.type = TOKEN_END;
        goto finish;
    }

    if (*c == '/')
        c++;

    if (*c == '\0') {
        tok.type = c - 1 == lx->str ? TOKEN_END : TOKEN_TRAILING;
        goto finish;
    }

    if (*c == '/')
        goto illegal;

    if ((*c == ':' && colon) || (*c == '<' && angle) || (*c == '{' && brace)) {
        tok.type = TOKEN_PARAM;
        c++;

        // Parameter names must begin with a character [a-zA-Z].
        if (!isalpha(*c))
            goto illegal;

    } else if (*c == '*') {
        c++;
        tok.type = *c == '\0' || *c == '/' ? TOKEN_WILDCARD : TOKEN_ILLEGAL;
        goto finish;
    } else {
        tok.type = TOKEN_LITERAL;
    }

    extra = 0;
    for (start = c; *c != '\0' && *c != '/'; c++) {
        if (tok.type == TOKEN_PARAM) {
            if ((*c == '>' && angle) || (*c == '}' && brace)) {
                extra = 1;
                break;
            }

            // After the first character, parameter names may also contain
            // digits and underscores.
            if (!isalnum(*c) && *c != '_')
                goto illegal;

            continue;
        }

        if (*c == '*' || *c == '#' || *c == '?' || isspace(*c) || iscntrl(*c))
            goto illegal;
    }

    // Measure before narrowing to token_t.length.
    length = c - start;

    // Guard against stupid sizes.
    if (length > LEXER_CHAR_LIMIT)
        goto illegal;

    // Save token string.
    tok.ptr = start;
    tok.length = length;

    c += extra;

finish:
    lx->cursor = c;
    return tok;

illegal:
    tok.type = TOKEN_ILLEGAL;
    tok.length = 0;
    tok.ptr = NULL;
    goto finish;
}
