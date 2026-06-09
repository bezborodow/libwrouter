#include "graph.h"
#include "terminal.h"
#include "wrouter.h"
#include "builder.h"
#include "router.h"
#include "helpers/error_helpers.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void print_graph(wrouter_t *router)
{
    for (uint16_t i = 0; i < router->graph_size; i++) {
        fprintf(stderr, "%04x: %04x\n", i, router->graph[i]);
    }
}

static void cb_null(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    (void)dispatch_ctx;
    (void)route_ctx;
    (void)params;

    return;
}

static void test_builder_free(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    wrouter_builder_free(builder);

    // Calling free on NULL is safe.
    wrouter_builder_free(NULL);
}

static void test_builder_add_route(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    wrouter_route_t route = { 0 };

    assert(wrouter_add_route(builder, "/users/:id", route) == WROUTER_OK);

    wrouter_builder_free(builder);
}

static void test_builder_add_handler(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    assert(wrouter_add_handler(builder, "/users/:id", cb_null) == WROUTER_OK);

    wrouter_builder_free(builder);
}

static void test_builder_add_handler_ctx(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    assert(wrouter_add_handler_ctx(builder, "/users/:id", cb_null, NULL) == WROUTER_OK);

    wrouter_builder_free(builder);
}

static void test_builder_add_context(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    assert(wrouter_add_context(builder, "/users/:id", NULL) == WROUTER_OK);

    wrouter_builder_free(builder);
}

static void test_builder_duplicate(void)
{
    wrouter_options_t options = { 0 };

    struct {
        const char *pattern;
        wrouter_error_t expected;
    } cases[] = { { "/users", WROUTER_ERR_DUPLICATE_ROUTE },
                  { "/users/", WROUTER_ERR_DUPLICATE_ROUTE },
                  { "/*", WROUTER_ERR_DUPLICATE_ROUTE },
                  { "/", WROUTER_ERR_DUPLICATE_ROUTE } };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {

        wrouter_builder_t *builder = wrouter_builder_create(options);
        assert(builder != NULL);

        // Base insert must succeed.
        assert(wrouter_add_context(builder, cases[i].pattern, NULL) == WROUTER_OK);

        // Duplicate insert must fail.
        ASSERT_ERROR(wrouter_add_context(builder, cases[i].pattern, NULL), cases[i].expected);

        // Subsequent calls must be denied.
        ASSERT_ERROR(wrouter_add_context(builder, cases[i].pattern, NULL),
                     WROUTER_ERR_BUILDER_CORRUPTED);

        wrouter_error_t err;
        wrouter_t *router = wrouter_compile(builder, &err);
        ASSERT_ERROR(err, WROUTER_ERR_BUILDER_CORRUPTED);
        assert(router == NULL);

        wrouter_builder_free(builder);
    }
}

static void test_builder_conflicts(void)
{
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = NULL;

    // Param vs literal.
    builder = wrouter_builder_create(options);
    assert(builder != NULL);
    assert(wrouter_add_context(builder, "/one/foo", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/one/:foo", NULL) ==
           WROUTER_ERR_PARAM_CONFLICTS_WITH_LITERAL);
    wrouter_builder_destroy(&builder);
    ;

    // Literal vs param.
    builder = wrouter_builder_create(options);
    assert(wrouter_add_context(builder, "/two/:foo", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/two/foo", NULL) ==
           WROUTER_ERR_LITERAL_CONFLICTS_WITH_PARAM);
    wrouter_builder_destroy(&builder);
    ;

    // Wildcard vs param.
    builder = wrouter_builder_create(options);
    assert(wrouter_add_context(builder, "/three/:foo", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/three/*", NULL) ==
           WROUTER_ERR_WILDCARD_CONFLICTS_WITH_PARAM);
    wrouter_builder_destroy(&builder);
    ;

    // Param vs wildcard.
    builder = wrouter_builder_create(options);
    assert(wrouter_add_context(builder, "/four/*", NULL) == WROUTER_OK);
    assert(wrouter_add_context(builder, "/four/:foo", NULL) ==
           WROUTER_ERR_PARAM_CONFLICTS_WITH_WILDCARD);

    wrouter_builder_free(builder);
}

static void test_builder_param_mismatch(void)
{
    wrouter_options_t options = { 0 };

    const char *ok_routes[] = { "/foo/:bar", "/foo/:bar/test", "/foo/:bar/test/",
                                "/foo/:bar/test/*" };

    const char *bad_routes[] = { "/foo/:baz", "/foo/:baz/test", "/foo/:baz/test/",
                                 "/foo/:baz/test/*" };

    const size_t ok_count = sizeof(ok_routes) / sizeof(ok_routes[0]);
    const size_t bad_count = sizeof(bad_routes) / sizeof(bad_routes[0]);

    for (size_t i = 0; i < bad_count; i++) {

        wrouter_builder_t *builder = wrouter_builder_create(options);
        assert(builder != NULL);

        // Prepopulate valid routes
        for (size_t j = 0; j < ok_count; j++) {
            assert(wrouter_add_context(builder, ok_routes[j], NULL) == WROUTER_OK);
        }

        // Inject offending route
        assert(wrouter_add_context(builder, bad_routes[i], NULL) ==
               WROUTER_ERR_PARAM_NAME_MISMATCH);

        wrouter_builder_free(builder);
    }
}

static void test_builder_wildcard_not_final(void)
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

static void test_builder_illegal_patterns(void)
{
    wrouter_options_t options = { 0 };

    const char *cases[] = {

        // Slashes in the wrong place
        "", "//", "///", "//foo/", "/foo//", "/foo//bar", "foo", "foo/",

        // Wildcards in the wrong place
        "/**", "/hello*", "/hello*world", "/*world",

        // Illegal parameter names
        "/:*", "/:$", "/:1", "/:_", "/:_a", "/:a:", "/:foo:"
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {

        wrouter_builder_t *builder = wrouter_builder_create(options);
        assert(builder != NULL);

        ASSERT_ERROR(wrouter_add_context(builder, cases[i], NULL), WROUTER_ERR_ILLEGAL_PATTERN);

        wrouter_builder_free(builder);
    }
}

static void test_builder_range_error_literal_edges(void)
{
    char buf[32];
    size_t i;
    bool range_error = false;
    wrouter_error_t err;
    wrouter_options_t options = { 0 };

    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    for (i = 0; i < UINT16_MAX; i++) {
        snprintf(buf, sizeof(buf), "/hello_%lu", i);
        err = wrouter_add_context(builder, buf, NULL);

        assert(err == WROUTER_OK || err == WROUTER_ERR_OUT_OF_RANGE);
        if (err == WROUTER_ERR_OUT_OF_RANGE) {
            range_error = true;
            break;
        }
    }
    assert(range_error);
    i--;

    if (i != NODE_MAX_CHILD_COUNT - 1) {
        fprintf(stderr, "Max child count Mismatch: got=%zu expected=%zd.\n", (size_t)i,
                NODE_MAX_CHILD_COUNT - 1);
    }
    assert(i == NODE_MAX_CHILD_COUNT - 1);

    wrouter_builder_free(builder);
}

static void test_builder_range_error_literal_edges_uint8_boundary(void)
{
    char buf[32];
    wrouter_error_t err;
    wrouter_options_t options = { 0 };

    wrouter_builder_t *builder = wrouter_builder_create(options);

    assert(builder != NULL);

    for (uint16_t i = 0; i < NODE_MAX_CHILD_COUNT; i++) {
        snprintf(buf, sizeof(buf), "/hello_%u", i);
        assert(wrouter_add_context(builder, buf, NULL) == WROUTER_OK);
    }

    // Reject the next edge before the public per-node limit is exceeded.
    snprintf(buf, sizeof(buf), "/hello_%zu", NODE_MAX_CHILD_COUNT);
    err = wrouter_add_context(builder, buf, NULL);
    ASSERT_ERROR(err, WROUTER_ERR_OUT_OF_RANGE);

    wrouter_builder_free(builder);
}

static void test_builder_range_error_literal_symbols(void)
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

static void test_builder_range_error_param_symbols(void)
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

static void test_builder_out_of_range_graph_size(void)
{
    // Choose a number that will exceed the limits.  (But not ridiculous,
    // otherwise tonnes of memory will be consumed.)
    enum { NI = 4, NJ = 55, NK = 100 };

    // Estimate how much space we are going to need for this graph.
    // TODO reimplement this.
#if 0
    size_t s = 0;

    s += sizeof(uint16_t); // Root node.
    s += sizeof(edge_t) * NI;
    s += sizeof(node_t) * NI;
    s += sizeof(edge_t) * NI * NJ;
    s += sizeof(node_t) * NI * NJ;
    s += sizeof(edge_t) * NI * NJ * NK;
    s += sizeof(node_t) * NI * NJ * NK;
    assert(s > UINT16_MAX);
#endif

    char buf[64];
    wrouter_error_t err;
    wrouter_options_t options = { 0 };

    wrouter_builder_t *builder = wrouter_builder_create(options);
    assert(builder != NULL);

    // Adding excessive routes will work ...
    for (uint16_t i = 0; i < NI; i++) {
        for (uint16_t j = 0; j < NJ; j++) {
            for (uint16_t k = 0; k < NK; k++) {
                snprintf(buf, sizeof(buf), "/a%u/b%u/c%u", i, j, k);

                err = wrouter_add_context(builder, buf, NULL);
                assert(err == WROUTER_OK);
            }
        }
    }

    // Check our estimate.
    graph_stats_t stats = { 0 };
    graph_stats(builder->root, &stats);
    // assert(stats.size == s); TODO

    // ... BUT compiling will run out of graph memory.
    wrouter_t *router = wrouter_compile(builder, &err);
    assert(router == NULL);
    assert(err == WROUTER_ERR_OUT_OF_RANGE);

    wrouter_builder_free(builder);
}

static void test_builder_consume(void)
{
    wrouter_error_t err;
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = NULL;
    wrouter_t *router = NULL;

    router = wrouter_consume(NULL, NULL);
    assert(router == NULL);

    router = wrouter_consume(&builder, NULL);
    assert(router == NULL);

    router = wrouter_consume(NULL, &err);
    assert(router == NULL);
    assert(err == WROUTER_ERR_NULL_ARGUMENT);

    router = wrouter_consume(&builder, &err);
    assert(router == NULL);
    assert(err == WROUTER_ERR_NULL_ARGUMENT);

    builder = wrouter_builder_create(options);
    assert(builder != NULL);

    wrouter_route_t route = { 0 };
    assert(wrouter_add_route(builder, "/users/:id", route) == WROUTER_OK);

    assert(builder != NULL);
    router = wrouter_consume(&builder, NULL);
    assert(builder == NULL);
    assert(router == NULL);

    builder = wrouter_builder_create(options);
    assert(builder != NULL);
    router = wrouter_consume(&builder, &err);
    assert(builder == NULL);
    assert(err == WROUTER_OK);
    assert(router != NULL);

    wrouter_destroy(&router);
    assert(router == NULL);
}

static void test_builder_compile_null_arguments(void)
{
    wrouter_error_t err;
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = NULL;
    wrouter_t *router = NULL;

    router = wrouter_compile(NULL, NULL);
    assert(router == NULL);

    router = wrouter_compile(NULL, &err);
    assert(router == NULL);
    assert(err == WROUTER_ERR_NULL_ARGUMENT);

    builder = wrouter_builder_create(options);
    assert(builder != NULL);

    router = wrouter_compile(builder, NULL);
    assert(router == NULL);

    wrouter_builder_free(builder);
}

void test_builder_destroy_null()
{
    wrouter_builder_t *builder = NULL;
    wrouter_builder_destroy(&builder);
    wrouter_builder_destroy(NULL);
}

void test_builder_empty_graph(void)
{
    fprintf(stderr, "-------------------\n");
    fprintf(stderr, "EMPTY\n");
    // Create builder.
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    assert(err == WROUTER_OK);
    wrouter_builder_free(builder);

    fprintf(stderr, "EMPTY GRAPH\n");
    print_graph(router);

    // First element of graph is zero (no flags or counts set.)
    assert(router->graph_size == 1);
    assert(router->graph[0] == 0);

    wrouter_free(router);
}

void test_builder_graph_with_wildcard(void)
{
    fprintf(stderr, "-------------------\n");
    // Create builder.
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    wrouter_route_t route = { NULL, NULL };

    // Add routes.
    assert(wrouter_add_route(builder, "/*", route) == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);

    uint16_t root_node = 0 | NODE_FLAG_HAS_WILDCARD;
    uint16_t wildcard_edge = 2;
    uint16_t wildcard_node = 0 | NODE_FLAG_TERMINAL;

    fprintf(stderr, "WILD\n");
    print_graph(router);

    assert(router->graph_size == 3);
    assert(router->graph[0] == root_node);
    assert(router->graph[1] == wildcard_edge);
    assert(router->graph[2] == wildcard_node);

    wrouter_free(router);
}

void test_builder_graph_with_root_terminal(void)
{
    fprintf(stderr, "-------------------\n");
    // Create builder.
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    int expected_ctx = 9999;
    wrouter_route_t expected_route = { NULL, &expected_ctx };

    // Add routes.
    assert(wrouter_add_route(builder, "/", expected_route) == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);

    fprintf(stderr, "ROOT TERMINAL\n");
    print_graph(router);

    uint16_t root_node = 0 | NODE_FLAG_TERMINAL;

    assert(router->graph_size == 1);
    assert(router->graph[0] == root_node);

    assert(router->terminals.count == 1);
    wrouter_route_t lookup_route = *terminal_lookup(&router->terminals, 0);
    assert(lookup_route.ctx == expected_route.ctx);
    assert(lookup_route.ctx == &expected_ctx);

    wrouter_free(router);
}

void test_builder_graph_with_two_literal_children(void)
{
    fprintf(stderr, "-------------------\n");
    fprintf(stderr, "TWO LITERALS\n");
    // Create builder.
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    const uint32_t expected_one = 11 * UINT16_MAX;
    const uint32_t expected_two = 22 * UINT16_MAX;

    // Add routes.
    assert(wrouter_add_context(builder, "/b_2_two", &expected_two) == 0);
    assert(wrouter_add_context(builder, "/a_1_one", &expected_one) == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);

    uint16_t root_node = 2;
    uint16_t one_sym = 1;
    uint16_t one_edge = 6;
    uint16_t two_sym = 2;
    uint16_t two_edge = 5;
    uint16_t one_node = 0 | NODE_FLAG_TERMINAL;
    uint16_t two_node = 0 | NODE_FLAG_TERMINAL;

    fprintf(stderr, "graph:\n");
    print_graph(router);

    assert(router->graph_size == 7);

    assert(router->graph[0] == root_node);
    assert(router->graph[1] == one_sym);
    assert(router->graph[2] == one_edge);
    assert(router->graph[3] == two_sym);
    assert(router->graph[4] == two_edge);
    assert(router->graph[5] == two_node);
    assert(router->graph[6] == one_node);

    assert(router->terminals.count == 2);
    wrouter_route_t *actual_one = terminal_lookup(&router->terminals, 6);
    wrouter_route_t *actual_two = terminal_lookup(&router->terminals, 5);
    assert(actual_two != NULL);
    assert((uint32_t *)actual_two->ctx == &expected_two);
    assert(actual_one != NULL);
    assert((uint32_t *)actual_one->ctx == &expected_one);

    wrouter_free(router);
}

void test_builder_graph_with_param(void)
{
    fprintf(stderr, "-------------------\n");
    // Create builder.
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    wrouter_route_t route = { NULL, NULL };

    // Add routes.
    assert(wrouter_add_route(builder, "/:param", route) == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);

    fprintf(stderr, "PARAM\n");
    print_graph(router);

    uint16_t root_node = 0 | NODE_FLAG_HAS_PARAM;
    uint16_t param_edge = 3;
    uint16_t param_sym = 1;
    uint16_t param_node = 0 | NODE_FLAG_TERMINAL;

    assert(router->graph_size == 4);
    assert(router->graph[0] == root_node);
    assert(router->graph[1] == param_sym);
    assert(router->graph[2] == param_edge);
    assert(router->graph[3] == param_node);

    wrouter_free(router);
}

void test_builder_graph_with_trailing(void)
{
    fprintf(stderr, "-------------------\n");
    // Create builder.
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    uint32_t expected_ctx = 9999;
    wrouter_route_t expected_route = { NULL, &expected_ctx};

    // Add routes.
    assert(wrouter_add_route(builder, "/trailing/", expected_route) == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);

    fprintf(stderr, "PARAM\n");
    print_graph(router);

    uint16_t root_node = 1;
    uint16_t literal_sym = 1;
    uint16_t literal_edge = 3;
    uint16_t literal_node = 0 | NODE_FLAG_HAS_TRAILING;
    uint16_t trailing_edge = 5;
    uint16_t trailing_node = 0 | NODE_FLAG_TERMINAL;

    assert(router->graph_size == 6);
    assert(router->graph[0] == root_node);
    assert(router->graph[1] == literal_sym);
    assert(router->graph[2] == literal_edge);
    assert(router->graph[3] == literal_node);
    assert(router->graph[4] == trailing_edge);
    assert(router->graph[5] == trailing_node);

    assert(router->terminals.count == 1);
    wrouter_route_t *actual_route = terminal_lookup(&router->terminals, 5);
    assert(actual_route != NULL);
    assert(actual_route->ctx == expected_route.ctx);
    const uint32_t *actual_ctx = actual_route->ctx;
    assert(*actual_ctx == 9999);

    wrouter_free(router);
}

void test_builder_graph_with_all_the_things(void)
{
    fprintf(stderr, "-------------------\n");
    // Create builder.
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    wrouter_route_t route = { NULL, NULL };

    // Add routes.
    assert(wrouter_add_route(builder, "/*", route) == 0);
    assert(wrouter_add_route(builder, "/aaaaa/", route) == 0);
    assert(wrouter_add_route(builder, "/aaaaa/:bbbbb", route) == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);

    uint16_t root_node = 1 | NODE_FLAG_HAS_WILDCARD;
    uint16_t wildcard_edge = 10;
    uint16_t a_sym = 1;
    uint16_t a_edge = 4;
    uint16_t a_node = 0 | NODE_FLAG_HAS_PARAM | NODE_FLAG_HAS_TRAILING;
    uint16_t b_sym = 1;
    uint16_t b_edge = 8;
    uint16_t t_edge = 9;
    uint16_t b_node = 0 | NODE_FLAG_TERMINAL;
    uint16_t t_node = 0 | NODE_FLAG_TERMINAL;
    uint16_t wildcard_node = 0 | NODE_FLAG_TERMINAL;

    fprintf(stderr, "WILD\n");
    print_graph(router);

    assert(router->graph_size == 11);
    assert(router->graph[0] == root_node);
    assert(router->graph[1] == wildcard_edge);
    assert(router->graph[2] == a_sym);
    assert(router->graph[3] == a_edge);
    assert(router->graph[4] == a_node);
    assert(router->graph[5] == b_sym);
    assert(router->graph[6] == b_edge);
    assert(router->graph[7] == t_edge);
    assert(router->graph[8] == b_node);
    assert(router->graph[9] == t_node);
    assert(router->graph[10] == wildcard_node);

    wrouter_free(router);
}


int main(void)
{
    test_builder_free();
    test_builder_add_route();
    test_builder_add_handler();
    test_builder_add_handler_ctx();
    test_builder_add_context();
    test_builder_duplicate();
    test_builder_conflicts();
    test_builder_param_mismatch();
    test_builder_wildcard_not_final();
    test_builder_illegal_patterns();
    test_builder_range_error_literal_edges();
    test_builder_range_error_literal_edges_uint8_boundary();
    test_builder_range_error_literal_symbols();
    test_builder_range_error_param_symbols();
    test_builder_out_of_range_graph_size();
    test_builder_consume();
    test_builder_compile_null_arguments();
    test_builder_destroy_null();
    test_builder_empty_graph();
    test_builder_graph_with_root_terminal();
    test_builder_graph_with_wildcard();
    test_builder_graph_with_two_literal_children();
    test_builder_graph_with_param();
    test_builder_graph_with_trailing();
    test_builder_graph_with_all_the_things();

    return 0;
}
