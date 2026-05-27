#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include <assert.h>
#include <string.h>

static void cb_test(void *dispatch_ctx, void *route_ctx, const wrouter_params_t params)
{
    // TODO Maybe do something here?
    (void)dispatch_ctx;
    (void)route_ctx;
    (void)params;
    return;
}

static void test_builder_add_route(void)
{
    wrouter_options_t options = { .param_syntax = WROUTER_SYNTAX_COLON,
                                  .fallback_handler = cb_test,
                                  .fallback_ctx = NULL };
    wrouter_builder_t *builder = wrouter_builder_create(options);

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
