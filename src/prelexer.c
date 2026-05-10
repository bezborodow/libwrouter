#include "prelexer.h"
#include "token.h"
#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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

    if (lx->cursor == NULL)
        return tok;

    if (*p != '\0') {
        char c = *p;

        if (c == '/')
            p++;

        if (*p == '/') {
            tok.type = TOKEN_ILLEGAL;
            return tok;
        }

        if (*p == ':') {
            tok.type = TOKEN_PARAM;
            p++;

            if (!isalpha(*p)) {
                tok.type = TOKEN_ILLEGAL;
                return tok;
            }
        } else {
            tok.type = TOKEN_LITERAL;
        }

        const char *start;
        for (start = p; *p != '\0' && *p != '/'; p++) {
            if (tok.type == TOKEN_PARAM && !isalnum(*p) && *p != '_')
                tok.type = TOKEN_ILLEGAL;
        }

        // TODO handle overflow.
        tok.length = p - start;

        lx->cursor = p;
    }

    if (tok.length == 0)
        tok.type = *p == '\0' ? TOKEN_END : TOKEN_ILLEGAL;

    return tok;
}
