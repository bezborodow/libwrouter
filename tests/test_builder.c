#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include <assert.h>
#include <string.h>

static void cb_test(void *dispatch_ctx, void *route_ctx, const wrouter_params_t *params) {
    return;
}

static void test_builder_add_route(void)
{
    wrouter_builder_t *builder = wrouter_builder_create(WROUTER_SYNTAX_BRACE);

    assert(builder != NULL);

    wrouter_route_t route = { 0 };
    route.handler = cb_test;

    wrouter_add_route(builder, "/users/{id}", route);

    wrouter_builder_free(builder);
}

int main(void)
{
    test_builder_add_route();

    return 0;
}
