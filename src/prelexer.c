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
    pretoken_t tok = {0};
    const char *p = lx->cursor;

    if (lx->cursor == NULL)
        return tok;

    while (*p != '\0') {
        char c = *p;

        // Skip separators.
        if (c == '/') {
            p++;
            continue;
        }

        if (*p == ':') {
            tok.type = TOKEN_PARAM;
            p++;
        } else {
            tok.type = TOKEN_LITERAL;
        }

        const char *start;
        if (!isalpha(*p)) {
            tok.type = TOKEN_ILLEGAL;
            return tok;
        }
        for (start = p; *p != '\0' && *p != '/'; p++) {
            if (!isalnum(*p) && *p != '_') {
                tok.type = TOKEN_ILLEGAL;
                return tok;
            }
        }

        // TODO handle overflow.
        tok.length = p - start;

        lx->cursor = p;
        break;
    }

    return tok;
}
