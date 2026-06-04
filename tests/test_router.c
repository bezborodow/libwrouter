#include "wrouter.h"
#include "router.h"
#include "params.h"
#include "symbol.h"
#include "builder.h"
#include "graph.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *pattern;
    const char *request;
    const wrouter_params_t *params;
    bool seen;
    int retained;
    int released;
} terminal_test_case_t;

static void cb_retain(const void *ctx)
{
    if (ctx == NULL)
        assert(0);

    ((terminal_test_case_t *)ctx)->retained++;
}

static void cb_release(const void *ctx)
{
    if (ctx == NULL)
        assert(0);

    ((terminal_test_case_t *)ctx)->released++;
}

static void cb_ignore(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    (void)dispatch_ctx;
    (void)route_ctx;
    (void)params;
    assert(0);
}

/**
 * Check that a route is called.
 *
 * Set the dispatch context boolean to true.
 *
 * If the route context is provided, then treat that as the number of parameters.
 * Otherwise, there should never be any parameters.
 */
static void cb_watch(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    uint32_t expected_parameter_count = 0;

    if (route_ctx != NULL)
        expected_parameter_count = *((uint32_t *)route_ctx);

    assert(params->count == expected_parameter_count);

    bool *seen = dispatch_ctx;
    *seen = true;
}

static void cb_test(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    terminal_test_case_t *dtc = dispatch_ctx;
    const terminal_test_case_t *rtc = route_ctx;

    assert(rtc == dtc);

    if (dtc->params != NULL && dtc->params->count) {
        assert(params->count == dtc->params->count);
        for (size_t i = 0; i < params->count; i++) {
            const wrouter_param_t *param_e = &dtc->params->base[i], *param = &params->base[i];

            if (param_e->length != param->length)
                fprintf(stderr, "%u %u\n", param_e->length, param->length);
            assert(param_e->length == param->length);
            assert(memcmp(param->value, param_e->value, param_e->length) == 0);
            assert(strcmp(param->name, param_e->name) == 0);

            // Test the wrouter_param and wrouter_iparam functions.
            assert(memcmp(wrouter_param(params, param->name)->value, param_e->value,
                          param_e->length) == 0);
            assert(memcmp(wrouter_iparam(params, i)->value, param_e->value, param_e->length) == 0);
        }
    }

    dtc->seen = true;
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

    if (seg->terminal != NULL)
        printf(" &");

    printf("\n");

    // Trailing-slash.
    if (seg->trailing != NULL) {
        for (int i = 0; i < depth + 1; i++)
            printf("  ");

        printf("/ &\n");
    }

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

        printf("* &\n");
    }
}

void builder_print_tree(const wrouter_builder_t *builder)
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
    wrouter_params_t document_params = {
        .base = (wrouter_param_t[]) {
            { WILDCARD_PARAM, "documents/schematic.pdf", 23 },
        },
        .count = 1
    };
    wrouter_params_t hello_params = {
        .base = (wrouter_param_t[]) {
            { WILDCARD_PARAM, "hello", 5 },
        },
        .count = 1
    };
    wrouter_params_t account_contact_credentials_params = {
        .base = (wrouter_param_t[]) {
            { "account_id", "200", 3 },
            { "account_contact_id", "300", 3 },
            { WILDCARD_PARAM, "letter_of_endorsement.pdf", 25 },
        },
        .count = 3
    };

    wrouter_params_t account_params = {
        .base = (wrouter_param_t[]) {
            { "account_id", "100", 3 },
        },
        .count = 1
    };

    wrouter_params_t account_contact_params = {
        .base = (wrouter_param_t[]) {
            { "account_id", "200", 3 },
            { "account_contact_id", "300", 3 },
        },
        .count = 2
    };

    wrouter_params_t project_params = {
        .base = (wrouter_param_t[]) {
            { "project_id", "400", 3 },
        },
        .count = 1
    };

    terminal_test_case_t cases[] = {
#if 1
        {
            .pattern = "/downloads/*",
            .request = "/downloads/documents/schematic.pdf",
            .params = &document_params,
        },
#endif
        {
            .pattern = "/downloads/",
            .request = "/downloads/",
        },
        {
            .pattern = "/downloads",
            .request = "/downloads",
        },
        {
            .pattern = "/",
            .request = "/",
        },
        {
            .pattern = "/*",
            .request = "/hello",
            .params = &hello_params,
        },
        {
            .pattern = "/accounts",
            .request = "/accounts",
        },
        {
            .pattern = "/accounts/create",
            .request = "/accounts/create",
        },
        {
            .pattern = "/account/<account_id>",
            .request = "/account/100",
            .params = &account_params,
        },
        {
            .pattern = "/account/<account_id>/edit",
            .request = "/account/100/edit",
            .params = &account_params,
        },
        {
            .pattern = "/account/<account_id>/projects",
            .request = "/account/100/projects",
            .params = &account_params,
        },
        {
            .pattern = "/account/<account_id>/contacts",
            .request = "/account/100/contacts",
            .params = &account_params,
        },
        {
            .pattern = "/account/<account_id>/contact/<account_contact_id>",
            .request = "/account/200/contact/300",
            .params = &account_contact_params,
        },
        {
            .pattern = "/account/<account_id>/contact/<account_contact_id>/credentials/*",
            .request = "/account/200/contact/300/credentials/letter_of_endorsement.pdf",
            .params = &account_contact_credentials_params,
        },
        {
            .pattern = "/projects",
            .request = "/projects",
        },
        {
            .pattern = "/projects/create",
            .request = "/projects/create",
        },
        {
            .pattern = "/project/<project_id>",
            .request = "/project/400",
            .params = &project_params,
        },
        {
            .pattern = "/project/<project_id>/edit",
            .request = "/project/400/edit",
            .params = &project_params,
        },
    };
    // clang-format on

    terminal_test_case_t fallback_tc = { 0 };

    // Create builder.
    wrouter_options_t options = {
        .param_syntax = WROUTER_SYNTAX_ANGLE,
        .fallback_handler = cb_ignore,
        .fallback_ctx = &fallback_tc,
        .retain = cb_retain,
        .release = cb_release,
    };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    wrouter_route_t route = { cb_test, NULL };

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
    for (size_t i = 0; i < n; i++) {
        assert(cases[i].retained == 1);
        assert(cases[i].released == 0);
    }
    assert(fallback_tc.retained == 1);
    assert(fallback_tc.released == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);

    assert(wrouter_route_count(router) == n);
    for (size_t i = 0; i < n; i++) {
        assert(cases[i].retained == 2);
        assert(cases[i].released == 0);
    }
    assert(fallback_tc.retained == 2);
    assert(fallback_tc.released == 0);

    // Free the builder.
    wrouter_builder_free(builder);
    assert(err == 0);

    assert(router != NULL);
    for (size_t i = 0; i < n; i++) {
        assert(cases[i].retained == 2);
        assert(cases[i].released == 1);
    }
    assert(fallback_tc.retained == 2);
    assert(fallback_tc.released == 1);

    // Dispatch.
    wrouter_dispatcher_t *dispatcher = wrouter_dispatcher_create(router);
    assert(dispatcher != NULL);
    for (size_t i = 0; i < n; i++) {
        assert(cases[i].seen == false);

        wrouter_dispatch(dispatcher, cases[i].request, &cases[i]);

        assert(cases[i].seen == true);
    }

    // Resolve.
    for (size_t i = 0; i < n; i++) {
        const terminal_test_case_t *tc = wrouter_resolve(dispatcher, cases[i].request);
        assert(tc == &cases[i]);
        const wrouter_params_t *params = wrouter_params(dispatcher);

        if (cases[i].params == NULL) {
            assert(params->count == 0);
            continue;
        }

        assert(cases[i].params->count == params->count);

        for (uint32_t j = 0; j < params->count; j++) {
            const wrouter_param_t *param_e = &cases[i].params->base[j];
            assert(param_e->length == params->base[j].length);
            assert(strcmp(param_e->name, params->base[j].name) == 0);
            assert(memcmp(param_e->value, params->base[j].value, param_e->length) == 0);
        }
    }

    wrouter_dispatcher_free(dispatcher);

    wrouter_free(router);

    for (size_t i = 0; i < n; i++) {
        assert(cases[i].retained == 2);
        assert(cases[i].released == 2);
    }
    assert(fallback_tc.retained == 2);
    assert(fallback_tc.released == 2);
}

void test_router_not_found(void)
{
    // Create builder.
    bool fallback_seen = false;
    wrouter_options_t options = {
        .param_syntax = WROUTER_SYNTAX_COLON,
        .fallback_handler = cb_watch,
        .fallback_ctx = NULL,
    };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    wrouter_route_t route = { cb_ignore, NULL };

    // Add routes.
    assert(wrouter_add_route(builder, "/hello/world", route) == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);
    assert(err == 0);
    assert(router != NULL);

    // Dispatch.
    wrouter_dispatcher_t *dispatcher = wrouter_dispatcher_create(router);
    assert(dispatcher != NULL);

    // Test ordinary not found.
    assert(!fallback_seen);
    wrouter_dispatch(dispatcher, "/this/does/not/exist", &fallback_seen);
    assert(fallback_seen);

    fallback_seen = false;
    wrouter_dispatch(dispatcher, "/hello/world/hello", &fallback_seen);
    assert(fallback_seen);

    // Test trailing slash not found.
    fallback_seen = false;
    wrouter_dispatch(dispatcher, "/this/does/not/exist/trailing/slash/", &fallback_seen);
    assert(fallback_seen);

    fallback_seen = false;
    wrouter_dispatch(dispatcher, "/hello/world/", &fallback_seen);
    assert(fallback_seen);

    // Test root not found.
    fallback_seen = false;
    wrouter_dispatch(dispatcher, "/", &fallback_seen);
    assert(fallback_seen);

    wrouter_dispatcher_free(dispatcher);
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
    bool wildcard_seen = false;

    // Add routes.
    const uint32_t expected_parameter_count = 1;
    assert(wrouter_add_handler_ctx(builder, "/*", cb_watch, &expected_parameter_count) == 0);
    assert(wrouter_add_handler(builder, "/literal", cb_ignore) == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);
    assert(err == 0);
    assert(router != NULL);

    // Dispatch.
    // Check that '/literal/go_to_wildcard' matches with '/*'.
    wrouter_dispatcher_t *dispatcher = wrouter_dispatcher_create(router);
    assert(dispatcher != NULL);
    wrouter_dispatch(dispatcher, "/literal/go_to_wildcard", &wildcard_seen);
    assert(wildcard_seen);

    wrouter_dispatcher_free(dispatcher);
    wrouter_free(router);
}

void test_router_top_wildcard_is_not_root(void)
{
    // Create builder.
    bool fallback_seen = false;
    wrouter_options_t options = {
        .param_syntax = WROUTER_SYNTAX_COLON,
        .fallback_handler = cb_watch,
        .fallback_ctx = NULL,
    };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Route handler.
    wrouter_route_t route = { cb_ignore, NULL };

    // Add routes.
    assert(wrouter_add_route(builder, "/*", route) == 0);

    // Compile.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);
    assert(err == 0);
    assert(router != NULL);

    // Dispatch.
    // Calling / should not match /*.
    wrouter_dispatcher_t *dispatcher = wrouter_dispatcher_create(router);
    assert(dispatcher != NULL);
    wrouter_dispatch(dispatcher, "/", &fallback_seen);
    assert(fallback_seen);

    wrouter_dispatcher_free(dispatcher);
    wrouter_free(router);
}

void test_router_empty_router(void)
{
    // Create builder.
    bool fallback_seen = false;
    wrouter_options_t options = {
        .param_syntax = WROUTER_SYNTAX_COLON,
        .fallback_handler = cb_watch,
        .fallback_ctx = NULL,
    };
    wrouter_builder_t *builder = wrouter_builder_create(options);

    // Compile without adding any routes.
    wrouter_error_t err;
    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);
    assert(err == 0);
    assert(router != NULL);

    // Dispatch.
    // Calling '/' should not match anything.
    wrouter_dispatcher_t *dispatcher = wrouter_dispatcher_create(router);
    assert(dispatcher != NULL);
    wrouter_dispatch(dispatcher, "/", &fallback_seen);
    assert(fallback_seen);

    wrouter_dispatcher_free(dispatcher);
    wrouter_free(router);
}

void test_router_free_null(void)
{
    // Calling on NULL will do nothing.
    wrouter_dispatcher_free(NULL);
    wrouter_free(NULL);
}

void test_router_destroy(void)
{
    wrouter_error_t err;
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = NULL;
    wrouter_t *router = NULL;

    builder = wrouter_builder_create(options);
    assert(builder != NULL);

    router = wrouter_consume(&builder, &err);
    assert(err == WROUTER_OK);
    assert(router != NULL);

    wrouter_destroy(&router);
    assert(router == NULL);

    wrouter_destroy(&router);
    assert(router == NULL);

    wrouter_destroy(NULL);
}

int main(void)
{
    test_router_basic();
    test_router_not_found();
    test_router_end_wildcard();
    test_router_top_wildcard_is_not_root();
    test_router_empty_router();
    test_router_free_null();
    test_router_destroy();

    return 0;
}
