#include "symbol.h"
#include <assert.h>
#include <string.h>

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

int main(void)
{
    test_symbol_append();
    test_symbol_table_growth();
    return 0;
}
