#include <stdint.h>
#include <stddef.h>

#ifndef WROUTER_SYMBOL_H
#define WROUTER_SYMBOL_H

typedef uint16_t symbol_t;

typedef const char * const symbol_table_entry_t;

typedef struct {
    symbol_table_entry_t *table;
    uint16_t count;
    uint16_t capacity;
} symbol_table_t;

typedef struct {
    symbol_table_t literals;
    symbol_table_t params;
} symbol_ctx_t;

symbol_t symbol_intern(symbol_table_t *tbl, const char *str, size_t len);

#endif
