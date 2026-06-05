#include "symbol.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void test_symbol_append(void)
{
    symbol_table_t tbl = { 0 };

    assert(symbol_append(&tbl, "hello", 5) != NULL);
    assert(symbol_append(&tbl, "world", 5) != NULL);

    // Check duplicates.
    const char *strptr = symbol_append(&tbl, "world", 5);
    assert(strptr != NULL);
    assert(strcmp(strptr, "world") == 0);

    assert(tbl.count == 2);

    assert(strcmp(tbl.base[0], "hello") == 0);
    assert(strcmp(tbl.base[1], "world") == 0);

    symbol_table_free(&tbl);
}

static void test_symbol_table_growth(void)
{
    symbol_table_t tbl = { 0 };

    size_t n = 2000;
    char buf[10];

    for (size_t i = 0; i < n; i++) {
        snprintf(buf, 9, "x%lu", i);
        const char *strptr = symbol_append(&tbl, buf, strlen(buf));
        assert(strptr != NULL);
        assert(strcmp(strptr, buf) == 0);
        assert(tbl.base[i] == strptr);
    }

    assert(tbl.count == n);
    assert(tbl.capacity >= n);

    symbol_table_free(&tbl);
}

static void test_symbol_compare(void)
{
    struct {
        const char *key;
        const char *sym;
        bool equal;
    } cases[] = {
        // clang-format off
        { "project",  "project",  true  },
        { "\xFF",     "\xFF",     true  },
        { "x",        "x",        true  },
        { "aaa",      "aaa",      true  },
        { "project/", "project",  false },
        { "user/",    "user",     false },
        { "x/",       "x",        false },
        { "a",        "aaa",      false },
        { "aaa",      "a",        false },
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
        // clang-format on
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const char *k = cases[i].key;
        const char *s = cases[i].sym;

        const char *const *a = &k;
        const char *const *b = &s;

        bool eq = (symbol_compare(a, b) == 0);
        assert(eq == cases[i].equal);
        eq = (symbol_compare(b, a) == 0);
        assert(eq == cases[i].equal);
    }
}
static void test_symbol_ncompare(void)
{
    struct {
        const char *key;
        const char *sym;
        bool equal;
    } cases[] = {
        // clang-format off
        { "project",  "project",  true  },
        { "\xFF",     "\xFF",     true  },
        { "x",        "x",        true  },
        { "aaa",      "aaa",      true  },
        { "project/", "project",  false },
        { "user/",    "user",     false },
        { "x/",       "x",        false },
        { "a",        "aaa",      false },
        { "aaa",      "a",        false },
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
        // clang-format on
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        size_t length = strlen(cases[i].key);
        char *str = malloc(length);
        str = memcpy(str, cases[i].key, length);
        struct symbol_key k = {
            .ptr = str,
            .length = length,
        };
        const char *s = cases[i].sym;

        struct symbol_key *a = &k;
        const char *const *b = &s;

        bool eq = (symbol_ncompare(a, b) == 0);
        free(str);
        assert(eq == cases[i].equal);
    }
}

void test_symbol_resolve(void)
{
    const char *symarr[] = {
        "admin", "create", "list", "project", "user", "x", "x1", "x2",
    };

    symbols_t symbols = { 0 };
    symbols.base = symarr;
    symbols.count = sizeof(symarr) / sizeof(symarr[0]);

    assert(symbol_resolve(&symbols, "admin") == 1);
    assert(symbol_resolve(&symbols, "create") == 2);
    assert(symbol_resolve(&symbols, "list") == 3);
    assert(symbol_resolve(&symbols, "project") == 4);
    assert(symbol_resolve(&symbols, "user") == 5);

    assert(symbol_resolve(&symbols, "x") == 6);
    assert(symbol_resolve(&symbols, "x1") == 7);
    assert(symbol_resolve(&symbols, "x2") == 8);

    assert(symbol_resolve(&symbols, "downloads") == 0);
    assert(symbol_resolve(&symbols, "projects") == 0);
    assert(symbol_resolve(&symbols, "project/") == 0);
    assert(symbol_resolve(&symbols, "user/") == 0);
}

void test_symbol_nresolve(void)
{
    size_t n;
    char *str = NULL;
    const char *symarr[] = {
        "aaaaa",
        "bbb",
    };
    symbols_t symbols = { 0 };
    symbols.base = symarr;
    symbols.count = sizeof(symarr) / sizeof(symarr[0]);

    n = 5;
    str = calloc(n, 1);
    memset(str, 'a', n);
    assert(symbol_nresolve(&symbols, str, n) == 1);
    free(str);

    n = 3;
    str = calloc(n, 1);
    memset(str, 'a', n);
    assert(symbol_nresolve(&symbols, str, n) == 0);
    free(str);

    n = 9;
    str = calloc(n, 1);
    memset(str, 'a', n);
    assert(symbol_nresolve(&symbols, str, n) == 0);
    free(str);

    n = 3;
    str = calloc(n, 1);
    memset(str, 'b', n);
    assert(symbol_nresolve(&symbols, str, n) == 2);
    free(str);
}

int main(void)
{
    test_symbol_append();
    test_symbol_table_growth();
    test_symbol_compare();
    test_symbol_ncompare();
    test_symbol_resolve();
    test_symbol_nresolve();
    return 0;
}

#if 0
TODO This prooves a segfault condition in symbol_compare.
    char *str = calloc(5, 1);
    char *strnt = "aaaaaaaaa";
    memset(str, 'a', 5);

    char **a = &str;
    char **b = &strnt;

    assert(symbol_compare(a, b) == 0);
    assert(symbol_compare(b, a) == 0);

    free(str);
#endif
