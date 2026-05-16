#include "symbol.h"
#include "arena.h"
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int symbol_table_next_slot(symbol_table_t *tbl, size_t len, size_t *slot)
{
    if (tbl->count + len > tbl->capacity) {
        size_t new_cap = tbl->capacity ? tbl->capacity * 2 : 1024;

        while (new_cap < tbl->count + len)
            new_cap *= 2;

        const char **new_base = realloc(tbl->base, new_cap * sizeof(*tbl->base));
        if (!new_base)
            return -ENOMEM;

        tbl->base = new_base;
        tbl->capacity = new_cap;
    }

    *slot = tbl->count;
    tbl->count += len;
    return 0;
}

int symbol_table_init(symbol_table_t *tbl)
{
    memset(tbl, 0, sizeof(*tbl));
}

int symbol_append(symbol_table_t *tbl, const char *str, size_t len)
{
    size_t slot;

    if (symbol_table_next_slot(tbl, 1, &slot) != 0)
        return -ENOMEM;

    char *a = arena_alloc(&tbl->arena, len + 1);
    if (a == NULL)
        return -ENOMEM;

    memcpy(a, str, len);
    a[len] = '\0';

    tbl->base[slot] = a;

    return 0;
}

void symbol_table_free(symbol_table_t *tbl)
{
    free(tbl->base);
    tbl->base = NULL;
    tbl->count = 0;
    tbl->capacity = 0;

    arena_free(&tbl->arena);
}
