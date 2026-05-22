#include "symbol.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>

static void test_symbol_append(void)
{
    symbol_table_t tbl;
    symbol_table_init(&tbl);

    assert(symbol_append(&tbl, "hello", 5) == 0);
    assert(symbol_append(&tbl, "world", 5) == 0);

    assert(tbl.count == 2);

    assert(strcmp(tbl.base[0], "hello") == 0);
    assert(strcmp(tbl.base[1], "world") == 0);

    symbol_table_free(&tbl);
}

static void test_symbol_table_growth(void)
{
    symbol_table_t tbl;
    symbol_table_init(&tbl);

    size_t n = 2000;

    for (size_t i = 0; i < n; i++)
        assert(symbol_append(&tbl, "x", 1) == 0);

    assert(tbl.count == n);
    assert(tbl.capacity >= n);

    for (size_t i = 0; i < n; i++)
        assert(strcmp(tbl.base[i], "x") == 0);

    symbol_table_free(&tbl);
}

static void test_symbol_compare(void)
{
    struct {
        const char *key;
        const char *sym;
        bool equal;
    } cases[] = {
        { "project",  "project",  true  },
        { "project/", "project",  true  },
        { "user/",    "user",     true  },
        { "x/",       "x",        true  },
        { "x",        "x",        true  },
        { "\xFF",     "\xFF",     true  },
        { "proj/",    "project",  false },
        { "x1",       "x2",       false },
        { "x2",       "x1",       false },
        { "x/",       "x2",       false },
        { "x",        "x10",      false },
        { "x10",      "x1",       false },
        { "x1",       "x10",      false },
        { "a",        "b",        false },
        { "b",        "a",        false },
        { "ab/",      "abc",      false },
        { "abc/",     "abd",      false },
        { "\xFF",     "A",        false },
        { "A",        "\xFF",     false },
        { "\x01",     "\xFF",     false },
        { "\xFF",     "\x01",     false },
    };

    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        const char *k = cases[i].key;
        const char *s = cases[i].sym;

        const char * const *a = &k;
        const char * const *b = &s;

        bool eq = (symbol_compare(a, b) == 0);
        assert(eq == cases[i].equal);
    }
}

void test_symbol_resolve(void)
{
    const char *symbols[] = {
        "admin",
        "create",
        "list",
        "project",
        "user",
        "x",
        "x1",
        "x2",
    };

    size_t n = sizeof(symbols) / sizeof(symbols[0]);

    assert(symbol_resolve("admin", symbols, n) == 1);
    assert(symbol_resolve("create", symbols, n) == 2);
    assert(symbol_resolve("list", symbols, n) == 3);
    assert(symbol_resolve("project", symbols, n) == 4);
    assert(symbol_resolve("user", symbols, n) == 5);

    assert(symbol_resolve("project/", symbols, n) == 4);
    assert(symbol_resolve("user/", symbols, n) == 5);

    assert(symbol_resolve("downloads", symbols, n) == 0);
    assert(symbol_resolve("projects", symbols, n) == 0);

    assert(symbol_resolve("x", symbols, n) == 6);
    assert(symbol_resolve("x1", symbols, n) == 7);
    assert(symbol_resolve("x2", symbols, n) == 8);
}

int main(void)
{
    test_symbol_append();
    test_symbol_table_growth();
    test_symbol_compare();
    test_symbol_resolve();
    return 0;
}
