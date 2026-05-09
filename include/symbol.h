#include <stdint.h>

#ifndef WROUTER_SYMBOL_H
#define WROUTER_SYMBOL_H

typedef const char * const symbol_table_entry_t;

typedef struct {
    symbol_table_entry_t *table;
    uint16_t count;
} symbol_table_t;

typedef struct {
    symbol_table_t literals;
    symbol_table_t params;
} symbol_ctx_t;

#endif
