#include "wrouter.h"
#include "token.h"
#include "symbol.h"
#include <stdint.h>
#include <stddef.h>

#ifndef WROUTER_LEXER_H
#define WROUTER_LEXER_H

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t symbol;
} token_t;

typedef struct {
    const char *cursor;
    size_t length;
    symbol_ctx_t *symbols;
} lexer_t;

void lexer_init(lexer_t *lx, const char *input, size_t length, symbol_ctx_t *symbols);

token_t lexer_next(lexer_t *lx);

#endif
