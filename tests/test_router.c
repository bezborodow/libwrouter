#include "wrouter.h"
#include "router.h"
#include "symbol.h"
#include "builder.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
    const char *pattern;
    const char *request;
    const wrouter_params_t *params;
} terminal_test_case_t;

static void cb_fail(void *dispatch_ctx, void *route_ctx, const wrouter_params_t *params)
{
    (void)dispatch_ctx;
    (void)route_ctx;
    (void)params;
    assert(0);
}

static void cb_test(void *dispatch_ctx, void *route_ctx, const wrouter_params_t *params)
{
    terminal_test_case_t *dtc = dispatch_ctx, *rtc = route_ctx;
    printf("REQUEST HANLDER CALLBACK\n");
    printf("Dispatch request: %s\n", dtc->request);
    printf("Route request:    %s\n", rtc->request);
    printf("Route pattern:    %s\n", rtc->pattern);
    assert(rtc == dtc);
}

static void print_route_node(const segment_t *seg, int depth, int is_param)
{
    // Indent.
    for (int i = 0; i < depth; i++)
        printf("  ");

    if (is_param)
        printf(":");

    // Current node.
    if (seg->str != NULL)
        printf("%.*s", seg->str_length, seg->str);
    else
        printf("/");

    if (seg->route.handler != NULL)
        printf(" &");

    printf("\n");

    // Literal children.
    for (uint16_t i = 0; i < seg->child_count; i++) {
        print_route_node(seg->children[i], depth + 1, 0);
    }

    // Param child.
    if (seg->spec_type == SPEC_PARAM && seg->special.param != NULL) {

        print_route_node(seg->special.param, depth + 1, 1);
    }

    // Wildcard route.
    if (seg->spec_type == SPEC_WILDCARD && seg->special.wildcard != NULL) {
        for (int i = 0; i < depth + 1; i++)
            printf("  ");

        printf("*");
        if (seg->special.wildcard->route.handler != NULL)
            printf(" &");
        printf("\n");
    }
}

void builder_print_tree(const struct builder *builder)
{
    if (builder == NULL || builder->root == NULL) {
        printf("(empty)\n");
        return;
    }

    print_route_node(builder->root, 0, 0);
}

void test_router_basic(void)
{
    // clang-format off
    wrouter_params_t account_params = {
        .base = (param_t[]) {
            { "account_id", "100" },
        },
        .count = 1
    };

    wrouter_params_t account_contact_params = {
        .base = (param_t[]) {
            { "account_id", "200" },
            { "account_contact_id", "300" },
        },
        .count = 2
    };

    wrouter_params_t project_params = {
        .base = (param_t[]) {
            { "project_id", "400" },
        },
        .count = 1
    };

    terminal_test_case_t cases[] = {
        {
            .pattern = "/downloads/*",
            .request = "/downloads/documents/schematic.pdf",
            .params = NULL
        },
        {
            .pattern = "/",
            .request = "/",
            .params = NULL
        },
        {
            .pattern = "/*",
            .request = "/hello",
            .params = NULL
        },
        {
            .pattern = "/accounts",
            .request = "/accounts",
            .params = NULL
        },
        {
            .pattern = "/accounts/create",
            .request = "/accounts/create",
            .params = NULL
        },
        {
            .pattern = "/account/<account_id>",
            .request = "/account/100",
            .params = &account_params
        },
        {
            .pattern = "/account/<account_id>/edit",
            .request = "/account/100/edit",
            .params = &account_params
        },
        {
            .pattern = "/account/<account_id>/projects",
            .request = "/account/100/projects",
            .params = &account_params
        },
        {
            .pattern = "/account/<account_id>/contacts",
            .request = "/account/100/contacts",
            .params = &account_params
        },
        {
            .pattern = "/account/<account_id>/contact/<account_contact_id>",
            .request = "/account/200/contact/300",
            .params = &account_contact_params
        },
        {
            .pattern = "/projects",
            .request = "/projects",
            .params = NULL
        },
        {
            .pattern = "/projects/create",
            .request = "/projects/create",
            .params = NULL
        },
        {
            .pattern = "/project/<project_id>",
            .request = "/project/400",
            .params = &project_params
        },
        {
            .pattern = "/project/<project_id>/edit",
            .request = "/project/400/edit",
            .params = &project_params
        },
    };
    // clang-format on

    // Create builder.
    wrouter_options_t options = {
        .param_syntax = WROUTER_SYNTAX_ANGLE,
        .fallback_handler = cb_fail,
        .fallback_ctx = NULL,
    };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    struct route route = { cb_test, NULL };

    // Add routes.
    size_t n = sizeof(cases) / sizeof(cases[0]);
    for (size_t i = 0; i < n; i++) {
        route.ctx = &cases[i];
        assert(wrouter_add_route(builder, cases[i].pattern, route) == 0);
    }

    builder_print_tree(builder);

    // Check stats.
    /*
    graph_stats_t stats = { 0 };
    graph_stats(builder->root, &stats);
    assert(stats.nodes == 18);
    assert(stats.edges == 5);
    assert(stats.symbolic_edges == 12);
    assert(stats.terminals == n);
    */

    // Compile.
    wrouter_t *router = wrouter_compile(builder);
    wrouter_builder_free(builder);

    // Dispatch.
    for (size_t i = 0; i < n; i++) {
        wrouter_dispatch(router, cases[i].request, &cases[i]);
    }

    wrouter_free(router);
}

int main(void)
{
    test_router_basic();
    return 0;
}
