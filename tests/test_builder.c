#include "graph.h"
#include "wrouter.h"
#include "builder.h"
#include "router.h"
#include "helpers/error_helpers.h"
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

static void test_conflicts(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    // Param vs literal.
    assert(wrouter_add_context(builder, "/one/foo", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/one/:foo", NULL) ==
           WROUTER_ERR_PARAM_CONFLICTS_WITH_LITERAL);

    // Literal vs param.
    assert(wrouter_add_context(builder, "/two/:foo", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/two/foo", NULL) ==
           WROUTER_ERR_LITERAL_CONFLICTS_WITH_PARAM);

    // Wildcard vs param.
    assert(wrouter_add_context(builder, "/three/:foo", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/three/*", NULL) ==
           WROUTER_ERR_WILDCARD_CONFLICTS_WITH_PARAM);

    // Param vs wildcard.
    assert(wrouter_add_context(builder, "/four/*", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/four/:foo", NULL) ==
           WROUTER_ERR_PARAM_CONFLICTS_WITH_WILDCARD);

    wrouter_builder_free(builder);
}

static void test_param_mismatch(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    // Parameters must have the same name at the same level.
    assert(wrouter_add_context(builder, "/foo/:bar", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/foo/:bar/test", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/foo/:bar/test/", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/foo/:bar/test/*", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/foo/:baz", NULL) == WROUTER_ERR_PARAM_NAME_MISMATCH);
    assert(wrouter_add_context(builder, "/foo/:baz/test", NULL) == WROUTER_ERR_PARAM_NAME_MISMATCH);
    assert(wrouter_add_context(builder, "/foo/:baz/test/", NULL) ==
           WROUTER_ERR_PARAM_NAME_MISMATCH);
    assert(wrouter_add_context(builder, "/foo/:baz/test/*", NULL) ==
           WROUTER_ERR_PARAM_NAME_MISMATCH);

    // But, at the same level of different parents is fine.
    assert(wrouter_add_context(builder, "/different-path1/:foo", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/different-path2/:bar", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/different-path3/:baz", NULL) == WROUTER_OK);

    wrouter_builder_free(builder);
}

static void test_wildcard_not_final(void)
{
    typedef struct {
        const char *pattern;
        wrouter_error_t expected;
    } test_case_t;

    static const test_case_t cases[] = {
        // OK.
        { "/*", WROUTER_OK },
        { "/one/*", WROUTER_OK },
        { "/two/:foo/*", WROUTER_OK },
        { "/three/:foo/bar/:baz/*", WROUTER_OK },

        // Not OK.
        { "/foo/*/bar/", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/foo/*/:bar/", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/foo/*/:bar/baz/:buzz", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/foo/*/:bar/baz/:buzz/", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/foo/*/", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/foo/:bar/*/", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/foo/:bar/baz/:buzz/*/", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/*/foo", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/*/foo/", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/*/", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/*/*", WROUTER_ERR_WILDCARD_NOT_FINAL },
        { "/*/*/", WROUTER_ERR_WILDCARD_NOT_FINAL },
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        wrouter_options_t options = { 0 };
        wrouter_builder_t *builder = wrouter_builder_create(options);
        assert(builder != NULL);

        ASSERT_ERROR(wrouter_add_context(builder, cases[i].pattern, NULL), cases[i].expected);

        wrouter_builder_free(builder);
    }
}

static void test_illegal_patterns(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    // Slashes in the wrong place.
    ASSERT_ERROR(wrouter_add_context(builder, "", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "//", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "///", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "//foo/", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/foo//", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/foo//bar", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "foo", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "foo/", NULL), WROUTER_ERR_ILLEGAL_PATTERN);

    // Wildcards in the wrong place.
    ASSERT_ERROR(wrouter_add_context(builder, "/**", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/hello*", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/hello*world", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/*world", NULL), WROUTER_ERR_ILLEGAL_PATTERN);

    // Illegal parameter names.
    ASSERT_ERROR(wrouter_add_context(builder, "/:*", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/:$", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/:1", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/:_", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/:_a", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/:a:", NULL), WROUTER_ERR_ILLEGAL_PATTERN);
    ASSERT_ERROR(wrouter_add_context(builder, "/:foo:", NULL), WROUTER_ERR_ILLEGAL_PATTERN);

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
    assert(i == NODE_CHILD_MAX);

    wrouter_builder_free(builder);
}

static void test_range_error_literal_symbols(void)
{
    wrouter_error_t err;
    wrouter_options_t options = { 0 };

    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    err = wrouter_add_context(builder, "/one", NULL);
    assert(err == WROUTER_OK);

    builder->literals.count = UINT16_MAX;

    err = wrouter_add_context(builder, "/two", NULL);
    ASSERT_ERROR(err, WROUTER_ERR_OUT_OF_RANGE);

    wrouter_builder_free(builder);
}

static void test_range_error_param_symbols(void)
{
    wrouter_error_t err;
    wrouter_options_t options = { 0 };

    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    err = wrouter_add_context(builder, "/:one", NULL);
    assert(err == WROUTER_OK);

    builder->params.count = UINT16_MAX;

    err = wrouter_add_context(builder, "/:one/:two", NULL);
    ASSERT_ERROR(err, WROUTER_ERR_OUT_OF_RANGE);

    wrouter_builder_free(builder);
}

static void test_out_of_range_graph_size(void)
{
    enum { NI = 2, NJ = 55, NK = 100 };

    char buf[64];
    wrouter_error_t err;
    wrouter_options_t options = { 0 };

    wrouter_builder_t *builder = wrouter_builder_create(options);
    assert(builder != NULL);

    for (uint16_t i = 0; i < NI; i++) {
        for (uint16_t j = 0; j < NJ; j++) {
            for (uint16_t k = 0; k < NK; k++) {
                snprintf(buf, sizeof(buf), "/a%u/b%u/c%u", i, j, k);

                err = wrouter_add_context(builder, buf, NULL);
                assert(err == WROUTER_OK);
            }
        }
    }

    wrouter_t *router = wrouter_compile(builder, &err);
    assert(router == NULL);
    assert(err == WROUTER_ERR_OUT_OF_RANGE);

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
    test_conflicts();
    test_param_mismatch();
    test_wildcard_not_final();
    test_illegal_patterns();
    test_range_error_literal_edges();
    test_range_error_literal_symbols();
    test_range_error_param_symbols();
    test_out_of_range_graph_size();

    return 0;
}
