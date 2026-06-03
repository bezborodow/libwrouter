#include "wrouter.h"
#include "builder.h"
#include "router.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void cb_null(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    (void)dispatch_ctx;
    (void)route_ctx;
    (void)params;

    return;
}

static void test_free(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    wrouter_builder_free(builder);

    // Calling free on NULL is safe.
    wrouter_builder_free(NULL);
}

static void test_add_route(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    wrouter_route_t route = { 0 };

    assert(wrouter_add_route(builder, "/users/:id", route) == WROUTER_OK);

    wrouter_builder_free(builder);
}

static void test_add_handler(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    assert(wrouter_add_handler(builder, "/users/:id", cb_null) == WROUTER_OK);

    wrouter_builder_free(builder);
}

static void test_add_handler_ctx(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    assert(wrouter_add_handler_ctx(builder, "/users/:id", cb_null, NULL) == WROUTER_OK);

    wrouter_builder_free(builder);
}

static void test_add_context(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    assert(wrouter_add_context(builder, "/users/:id", NULL) == WROUTER_OK);

    wrouter_builder_free(builder);
}

static void test_duplicate(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    // Segment literal duplicate.
    assert(wrouter_add_context(builder, "/users", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/users", NULL) == WROUTER_ERR_DUPLICATE_ROUTE);

    // Trailing duplicate.
    assert(wrouter_add_context(builder, "/users/", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/users/", NULL) == WROUTER_ERR_DUPLICATE_ROUTE);

    // Wildcard duplicate.
    assert(wrouter_add_context(builder, "/*", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/*", NULL) == WROUTER_ERR_DUPLICATE_ROUTE);

    // Root duplicate.
    assert(wrouter_add_context(builder, "/", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/", NULL) == WROUTER_ERR_DUPLICATE_ROUTE);

    wrouter_builder_free(builder);
}

static void test_range_error_literal_edges(void)
{
    char buf[32];
    uint16_t i;
    bool range_error = false;
    wrouter_error_t err;
    wrouter_options_t options = { 0 };

    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    for (i = 0; i < UINT16_MAX; i++) {
        snprintf(buf, sizeof(buf), "/hello_%u", i);
        err = wrouter_add_context(builder, buf, NULL);

        assert(err == WROUTER_OK || err == WROUTER_ERR_OUT_OF_RANGE);
        if (err == WROUTER_ERR_OUT_OF_RANGE) {
            range_error = true;
            break;
        }
    }
    assert(range_error);
    assert(i == UINT8_MAX);

    wrouter_builder_free(builder);
}

int main(void)
{
    test_free();
    test_add_route();
    test_add_handler();
    test_add_handler_ctx();
    test_add_context();
    test_duplicate();
    test_range_error_literal_edges();

    return 0;
}
