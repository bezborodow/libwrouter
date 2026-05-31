#include "wrouter.h"
#include "symbol.h"
#include "params.h"
#include <string.h>
#include <assert.h>

void test_params_copy(void)
{
    wrouter_params_snapshot_t *snapshot;

    // Create params in its own scope.
    {
        struct params params = { 0 };
        symbol_table_t table = { 0 };
        wrouter_param_t *param = NULL;

        symbol_table_init(&table);

        const char *str0 = "param0";
        const char *str1 = "param1";

        const char *sym0 = symbol_append(&table, str0, strlen(str0));
        const char *sym1 = symbol_append(&table, str1, strlen(str1));

        params.count = 2;
        params_alloc(&params, 2);

        // Simulate the lexer str pointers into the path.
        const char *path = "/path/value0/test/value1";

        param = &params.base[0];
        param->name = sym0;
        param->value = strstr(path, "value0");
        param->length = 6;

        param = &params.base[1];
        param->name = sym1;
        param->value = strstr(path, "value1");
        param->length = 6;

        // Do a deep copy.
        snapshot = wrouter_params_copy(&params);

        // Free everything; there should be no references into these objects
        // beyond this point.
        params_free(&params);
        symbol_table_free(&table);
    }

    // Check that copied params are accessible as null-terminated strings.
    assert(snapshot->params.base != NULL);
    assert(snapshot->params.count == 2);
    assert(strcmp(snapshot->params.base[0].name, "param0") == 0);
    assert(strcmp(snapshot->params.base[0].value, "value0") == 0);
    assert(strcmp(snapshot->params.base[1].name, "param1") == 0);
    assert(strcmp(snapshot->params.base[1].value, "value1") == 0);

    wrouter_snapshot_free(snapshot);
}

int main(void)
{
    test_params_copy();

    return 0;
}
