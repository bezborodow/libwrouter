#include "wrouter.h"
#include "builder.h"
#include <assert.h>
#include <string.h>

static void test_builder_add_route(void)
{
    wrouter_builder_t *builder = wrouter_builder_create(WROUTER_SYNTAX_BRACE);

    wrouter_builder_free(builder);
}

int main(void)
{
    test_builder_add_route();

    return 0;
}
