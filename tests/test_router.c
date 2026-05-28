#include "wrouter.h"
#include "router.h"
#include "symbol.h"
#include "builder.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

typedef struct {
    const char *pattern;
    const char *request;
    const wrouter_params_t *params;
    bool seen;
} terminal_test_case_t;

static void cb_ignore(void *dispatch_ctx, void *route_ctx, const wrouter_params_t *params)
{
    (void)dispatch_ctx;
    (void)route_ctx;
    (void)params;
    assert(0);
}

static void cb_watch(void *dispatch_ctx, void *route_ctx, const wrouter_params_t *params)
{
    (void)dispatch_ctx;
    assert(params->count == 0);

    bool *seen = route_ctx;
    *seen = true;
}

static void cb_test(void *dispatch_ctx, void *route_ctx, const wrouter_params_t *params)
{
    terminal_test_case_t *dtc = dispatch_ctx, *rtc = route_ctx;
    assert(rtc == dtc);

    if (dtc->params != NULL && dtc->params->count) {
        assert(params->count == dtc->params->count);
        for (size_t i = 0; i < params->count; i++) {
            const param_t *param_e = &dtc->params->items[i], *param = &params->items[i];

            assert(param_e->length == param->length);
            assert(memcmp(param->value, param_e->value, param_e->length) == 0);
            assert(strcmp(param->name, param_e->name) == 0);
        }
    }

    rtc->seen = true;
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
        .items = {
            { "account_id", "100", 3 },
        },
        .count = 1
    };

    wrouter_params_t account_contact_params = {
        .items = {
            { "account_id", "200", 3 },
            { "account_contact_id", "300", 3 },
        },
        .count = 2
    };

    wrouter_params_t project_params = {
        .items = {
            { "project_id", "400", 3 },
        },
        .count = 1
    };

    terminal_test_case_t cases[] = {
        {
            .pattern = "/downloads/*",
            .request = "/downloads/documents/schematic.pdf",
            .params = NULL,
            .seen = false,
        },
        {
            .pattern = "/downloads/",
            .request = "/downloads/",
            .params = NULL,
            .seen = false,
        },
        {
            .pattern = "/",
            .request = "/",
            .params = NULL,
            .seen = false,
        },
        {
            .pattern = "/*",
            .request = "/hello",
            .params = NULL,
            .seen = false,
        },
        {
            .pattern = "/accounts",
            .request = "/accounts",
            .params = NULL,
            .seen = false,
        },
        {
            .pattern = "/accounts/create",
            .request = "/accounts/create",
            .params = NULL,
            .seen = false,
        },
        {
            .pattern = "/account/<account_id>",
            .request = "/account/100",
            .params = &account_params,
            .seen = false,
        },
        {
            .pattern = "/account/<account_id>/edit",
            .request = "/account/100/edit",
            .params = &account_params,
            .seen = false,
        },
        {
            .pattern = "/account/<account_id>/projects",
            .request = "/account/100/projects",
            .params = &account_params,
            .seen = false,
        },
        {
            .pattern = "/account/<account_id>/contacts",
            .request = "/account/100/contacts",
            .params = &account_params,
            .seen = false,
        },
        {
            .pattern = "/account/<account_id>/contact/<account_contact_id>",
            .request = "/account/200/contact/300",
            .params = &account_contact_params,
            .seen = false,
        },
        {
            .pattern = "/account/<account_id>/contact/<account_contact_id>/credentials/*",
            .request = "/account/200/contact/300/credentials/letter_of_endorsement.pdf",
            .params = &account_contact_params,
            .seen = false,
        },
        {
            .pattern = "/projects",
            .request = "/projects",
            .params = NULL,
            .seen = false,
        },
        {
            .pattern = "/projects/create",
            .request = "/projects/create",
            .params = NULL,
            .seen = false,
        },
        {
            .pattern = "/project/<project_id>",
            .request = "/project/400",
            .params = &project_params,
            .seen = false,
        },
        {
            .pattern = "/project/<project_id>/edit",
            .request = "/project/400/edit",
            .params = &project_params,
            .seen = false,
        },
    };
    // clang-format on

    // Create builder.
    bool fallback_seen = false;
    wrouter_options_t options = {
        .param_syntax = WROUTER_SYNTAX_ANGLE,
        .fallback_handler = cb_watch,
        .fallback_ctx = &fallback_seen,
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
    graph_stats_t stats = { 0 };
    graph_stats(builder->root, &stats);
    assert(stats.terminals == n);

    // Compile.
    wrouter_t *router = wrouter_compile(builder);
    wrouter_builder_free(builder);

    // Dispatch.
    for (size_t i = 0; i < n; i++) {
        assert(cases[i].seen == false);

        wrouter_dispatch(router, cases[i].request, &cases[i]);

        assert(cases[i].seen == true);
    }

    // Fallback handler should not have been called.
    assert(!fallback_seen);

    wrouter_free(router);
}

void test_router_not_found(void)
{
    // Create builder.
    bool fallback_seen = false;
    wrouter_options_t options = {
        .param_syntax = WROUTER_SYNTAX_COLON,
        .fallback_handler = cb_watch,
        .fallback_ctx = &fallback_seen,
    };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    struct route route = { cb_ignore, NULL };

    // Add routes.
    assert(wrouter_add_route(builder, "/hello/world", route) == 0);

    builder_print_tree(builder);

    // Compile.
    wrouter_t *router = wrouter_compile(builder);
    wrouter_builder_free(builder);

    // Dispatch.
    assert(!fallback_seen);
    wrouter_dispatch(router, "/this/does/not/exist", NULL);
    assert(fallback_seen);

    fallback_seen = false;
    wrouter_dispatch(router, "/hello/world/hello", NULL);
    assert(fallback_seen);

    fallback_seen = false;
    wrouter_dispatch(router, "/", NULL);
    assert(fallback_seen);

    wrouter_free(router);
}

void test_router_end_wildcard(void)
{
    // Create builder.
    wrouter_options_t options = {
        .param_syntax = WROUTER_SYNTAX_COLON,
        .fallback_handler = cb_ignore,
        .fallback_ctx = NULL,
    };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    bool wilcard_seen = false;
    struct route watch_route = { cb_watch, &wilcard_seen };
    struct route ignore_route = { cb_ignore, NULL };

    // Add routes.
    assert(wrouter_add_route(builder, "/*", watch_route) == 0);
    assert(wrouter_add_route(builder, "/literal", ignore_route) == 0);

    builder_print_tree(builder);

    // Compile.
    wrouter_t *router = wrouter_compile(builder);
    wrouter_builder_free(builder);

    // Dispatch.
    wrouter_dispatch(router, "/literal/go_to_wildcard", NULL);
    assert(wilcard_seen);

    wrouter_free(router);
}

void test_router_top_wildcard_is_not_root(void)
{
    // Create builder.
    bool fallback_seen = false;
    wrouter_options_t options = {
        .param_syntax = WROUTER_SYNTAX_COLON,
        .fallback_handler = cb_watch,
        .fallback_ctx = &fallback_seen,
    };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    struct route route = { cb_ignore, NULL };

    // Add routes.
    assert(wrouter_add_route(builder, "/*", route) == 0);

    builder_print_tree(builder);

    // Compile.
    wrouter_t *router = wrouter_compile(builder);
    wrouter_builder_free(builder);

    // Dispatch.
    // Calling / should not match /*.
    wrouter_dispatch(router, "/", NULL);
    assert(fallback_seen);

    wrouter_free(router);
}

void test_router_free_null(void)
{
    // Calling on NULL will do nothing.
    wrouter_free(NULL);
}

int main(void)
{
    test_router_basic();
    test_router_not_found();
    test_router_end_wildcard();
    test_router_top_wildcard_is_not_root();
    test_router_free_null();

    return 0;
}
