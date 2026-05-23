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

int symbol_table_init(symbol_table_t *tbl);
int symbol_append(symbol_table_t *tbl, const char *str, size_t length);
void symbol_table_free(symbol_table_t *tbl);

size_t symbol_resolve(const char *key, const char **base, size_t nmemb);
int symbol_compare(const void *a, const void *b);

#endif
