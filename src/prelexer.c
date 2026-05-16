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

void prelexer_load(prelexer_t *lx, const char *input)
{
    lx->cursor = input;
}

pretoken_t prelexer_next(prelexer_t *lx)
{
    pretoken_t tok = { 0 };
    const char *p = lx->cursor;

    const bool angle = lx->param_syntax == WROUTER_SYNTAX_ANGLE;
    const bool brace = lx->param_syntax == WROUTER_SYNTAX_BRACE;
    const bool colon = lx->param_syntax == WROUTER_SYNTAX_COLON;

    if (lx->cursor == NULL)
        goto finish;

    if (*p == '\0') {
        tok.type = TOKEN_END;
        goto finish;
    }

    char c = *p;

    if (c == '/') {
        p++;
    }

    if (*p == '\0') {
        tok.type = TOKEN_END;
        goto finish;
    }

    if (*p == '/') {
        tok.type = TOKEN_ILLEGAL;
        goto finish;
    }

    if ((*p == ':' && colon) || (*p == '<' && angle) || (*p == '{' && brace)) {
        tok.type = TOKEN_PARAM;
        p++;

        if (!isalpha(*p)) {
            tok.type = TOKEN_ILLEGAL;
            goto finish;
        }
    } else {
        tok.type = TOKEN_LITERAL;
    }

    size_t extra = 0;
    const char *start;
    for (start = p; *p != '\0' && *p != '/'; p++) {
        if (tok.type == TOKEN_PARAM) {
            if ((*p == '>' && angle) || (*p == '}' && brace)) {
                extra = 1;
                break;
            }

            if (!isalnum(*p) && *p != '_') {
                tok.type = TOKEN_ILLEGAL;
                goto finish;
            }
        }
    }

    // TODO handle overflow.
    tok.length = p - start;
    p += extra;

finish:
    lx->cursor = p;

    return tok;
}
