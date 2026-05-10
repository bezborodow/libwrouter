#include "arena.h"
#include <stdint.h>
#include <stddef.h>

#ifndef WROUTER_SYMBOL_H
#define WROUTER_SYMBOL_H

typedef uint16_t symbol_t;

typedef struct {
    const char **base;
    size_t count;
    size_t capacity;
    arena_t arena;
} symbol_table_t;

typedef struct {
    symbol_table_t literals;
    symbol_table_t params;
} symbol_ctx_t;

int symbol_table_init(symbol_table_t *tbl);
int symbol_append(symbol_table_t *tbl, const char *str, size_t len);
void symbol_table_free(symbol_table_t *tbl);

#endif
