#pragma once
#include "arena.h"
#include "wrouter.h"
#include <stdint.h>
#include <stddef.h>

typedef uint16_t symbol_t;

typedef struct {
    const char **base;
    size_t count;
    size_t capacity;
    arena_t arena;
} symbol_table_t;

typedef struct symbols {
    const char **base;
    char *region;
    uint32_t count;
} symbols_t;

void symbol_table_init(symbol_table_t *tbl);
const char *symbol_append(symbol_table_t *tbl, const char *str, size_t length);
void symbol_table_free(symbol_table_t *tbl);

size_t symbol_resolve(const char *key, const char **base, size_t nmemb);
int symbol_compare(const void *a, const void *b);
wrouter_error_t symbol_compile(const symbol_table_t *tbl, symbols_t *sym);
void symbols_free(symbols_t *symbols);
