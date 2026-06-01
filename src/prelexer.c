#include "prelexer.h"
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
    // Enforce all patterns begin with '/'.
    if (*pattern != '/') {
        memset(lx, 0, sizeof(*lx));
        return;
    }

    lx->str = pattern;
    lx->cursor = pattern;
}

token_t prelexer_next(prelexer_t *lx)
{
    token_t tok = { 0 };
    const char *c = lx->cursor;

    const bool angle = lx->param_syntax == WROUTER_SYNTAX_ANGLE;
    const bool brace = lx->param_syntax == WROUTER_SYNTAX_BRACE;
    const bool colon = lx->param_syntax == WROUTER_SYNTAX_COLON;
    size_t extra;
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

    if (*c == '/') {
        tok.type = TOKEN_ILLEGAL;
        goto finish;
    }

    if ((*c == ':' && colon) || (*c == '<' && angle) || (*c == '{' && brace)) {
        tok.type = TOKEN_PARAM;
        c++;

        if (!isalpha(*c)) {
            tok.type = TOKEN_ILLEGAL;
            goto finish;
        }
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

            if (!isalnum(*c) && *c != '_') {
                tok.type = TOKEN_ILLEGAL;
                goto finish;
            }
        }
    }

    // TODO handle overflow.
    tok.ptr = start;
    tok.length = c - start;

    if (tok.length == 1 && start[0] == '*') {
        tok.type = TOKEN_WILDCARD;
        tok.length = 0;
    }

    c += extra;

finish:
    lx->cursor = c;

    return tok;
}
