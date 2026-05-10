#include "symbol.h"
#include "arena.h"
#include <stddef.h>
#include <errno.h>

static symbol_table_entry_t *symbol_table_next_slot(symbol_table_t *tbl, size_t len)
{
    if (tbl->count + len > tbl->capacity) {
        size_t new_cap = tbl->capacity ? tbl->capacity * 2 : 1024;

        while (new_cap < tbl->count + len)
            new_cap *= 2;

        symbol_table_entry_t *new_base = realloc(tbl->base, new_cap*sizeof(*tbl->base));
        if (!new_base)
            return NULL;

        tbl->base = new_base;
        tbl->capacity = new_cap;
    }

    symbol_table_entry_t *out = &tbl->base[tbl->count];
    tbl->count += len;
    return out;
}

int symbol_append(symbol_table_t *tbl, const char *str, size_t len)
{
    char *row = symbol_table_next_slot(tbl, 1);
    if (row == NULL) {
        return -ENOMEM;
    }

    char *a = arena_alloc(tbl->arena, len + 1);
    if (a == NULL) {
        return -ENOMEM;
    }
    memcpy(a, str, len);
    a[len] = '\0';

    *row = a;

    return 0;
}

void symbol_table_free(symbol_table_t *tbl)
{
    free(tbl->base);
    tbl->count = 0;
    tbl->capacity = 0;

    arena_free(tbl->arena);
}
