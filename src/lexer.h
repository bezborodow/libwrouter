#include "wrouter.h"
#include "token.h"
#include "symbol.h"
#include <stdint.h>
#include <stddef.h>

#ifndef WROUTER_LEXER_H
#define WROUTER_LEXER_H

typedef struct {
    const char *str;
    const char *cursor;
    size_t length;
} lexer_t;

void lexer_load(lexer_t *lx, const char *request, size_t length);

token_t lexer_next(lexer_t *lx);

#endif
