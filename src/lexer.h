#pragma once
#include "token.h"
#include "symbol.h"
#include "wrouter.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    const char *str;
    const char *cursor;
    size_t length;
} lexer_t;

void lexer_load(lexer_t *lx, const char *request, size_t length);

token_t lexer_next(lexer_t *lx);
