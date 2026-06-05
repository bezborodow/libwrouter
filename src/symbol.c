#include "symbol.h"
#include "arena.h"
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

void symbol_table_init(symbol_table_t *tbl)
{
    memset(tbl, 0, sizeof(*tbl));
}

const char *symbol_append(symbol_table_t *tbl, const char *str, size_t len)
{
    // Check for duplicates.
    for (size_t i = 0; i < tbl->count; i++)
        if (strncmp(tbl->base[i], str, len) == 0 && tbl->base[i][len] == '\0')
            return tbl->base[i];

    // Get a slot in the table for the string pointer.
    size_t slot;
    if (symbol_table_next_slot(tbl, 1, &slot) != 0)
        return NULL;

    // Allocate space in the arena for the string.
    char *a = arena_alloc(&tbl->arena, len + 1);
    if (a == NULL)
        return NULL;

    // Copy the string into the arena, ensuring it is null-terminated.
    memcpy(a, str, len);
    a[len] = '\0';

    // Append the string pointer.
    tbl->base[slot] = a;

    return a;
}

void symbol_table_free(symbol_table_t *tbl)
{
    free(tbl->base);
    tbl->base = NULL;
    tbl->count = 0;
    tbl->capacity = 0;

    arena_free(&tbl->arena);
}

int symbol_ncompare(const void *a, const void *b)
{
    const struct symbol_key *nkey = (const struct symbol_key *)a;
    const char *sym = *(const char **)b;
    const char *key = nkey->ptr;

    for (size_t i = 0; ; i++, key++, sym++) {
        if (!(i < nkey->length)) {
            if (!*sym)
                return 0;
            return - (unsigned char)*sym;
        }

        if (*key != *sym)
            goto miss;
    }

    return 0;

miss:
    return (unsigned char)*key - (unsigned char)*sym;
}

int symbol_compare(const void *a, const void *b)
{
    const char *key = *(const char **)a;
    const char *sym = *(const char **)b;

    for (; *key && *sym; key++, sym++)
        if (*key != *sym)
            goto miss;

    if (!*key && !*sym)
        return 0;

miss:
    return (unsigned char)*key - (unsigned char)*sym;
}

static int strpcmp(const void *p1, const void *p2)
{
    return strcmp(*(const char **)p1, *(const char **)p2);
}

/**
 * Compiles an alphabetically sorted symbol list from a symbol strings table.
 *
 * References its own memory region of character strings.
 *
 * @param tbl Symbol strings table.
 * @param sym Symbol list.
 */
wrouter_error_t symbol_compile(const symbol_table_t *tbl, symbols_t *sym)
{
    sym->count = tbl->count;

    // Check for an empty symbol table, which is valid, and return immediately
    // without any changes. This assumes that the symbol list is initialised
    // to zeros.
    if (!sym->count)
        return WROUTER_OK;

    // Allocate space for the string pointers.
    sym->base = malloc(sizeof(char *) * sym->count);
    if (sym->base == NULL)
        goto no_memory;

    // Allocate space for the strings in a contiguous memory region.
    sym->region = malloc(arena_used(&tbl->arena));
    if (sym->region == NULL) {
        goto no_memory;
    }

    // Copy and sort string pointers by string contents.
    memcpy(sym->base, tbl->base, sizeof(char *) * sym->count);
    qsort(sym->base, sym->count, sizeof(char *), strpcmp);

    // Copy strings from the arena and update the string pointers.
    for (size_t cursor = 0, i = 0; i < sym->count; i++) {
        size_t n = strlen(sym->base[i]) + 1;

        // Copy string.
        memcpy(sym->region + cursor, sym->base[i], n);

        // Update pointer to point to the copied string!
        sym->base[i] = sym->region + cursor;

        cursor += n;
    }

    return WROUTER_OK;

no_memory:
    symbols_free(sym);
    return WROUTER_ERR_NO_MEMORY;
}

size_t symbol_nresolve(const symbols_t *symbols, const char *key, size_t n)
{
    const struct symbol_key k = {
        .ptr = key,
        .length = n,
    };

    size_t nmemb = symbols->count;
    const char **res, **base = symbols->base;

    // Binary search of the sorted symbol array.
    res = bsearch(&k, base, nmemb, sizeof(char *), symbol_ncompare);

    // Zero indicates that a symbol was not found. Therefore, increment the
    // index by one if there was a match; return zero otherwise.
    return res ? res - base + 1 : 0;
}

size_t symbol_resolve(const symbols_t *symbols, const char *key)
{
    const char *k = key;
    size_t nmemb = symbols->count;
    const char **res, **base = symbols->base;

    // Binary search of the sorted symbol array.
    res = bsearch(&k, base, nmemb, sizeof(char *), symbol_compare);

    // Zero indicates that a symbol was not found. Therefore, increment the
    // index by one if there was a match; return zero otherwise.
    return res ? res - base + 1 : 0;
}

void symbols_free(symbols_t *symbols)
{
    if (symbols == NULL)
        return;

    free(symbols->region);
    free(symbols->base);
}
