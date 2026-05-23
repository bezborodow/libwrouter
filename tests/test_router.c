#include "wrouter.h"
#include "router.h"
#include "symbol.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>


void test_router_basic(void)
{
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

    typedef struct {
        const char *route;
        const char *request;
        const wrouter_params_t *params;
    } terminal_test_case_t;

    terminal_test_case_t cases[] = {
        {
            .route = "/",
            .request = "/",
            .params = NULL
        },
        {
            .route = "/accounts",
            .request = "/accounts",
            .params = NULL
        },
        {
            .route = "/accounts/create",
            .request = "/accounts/create",
            .params = NULL
        },
        {
            .route = "/account/<account_id>",
            .request = "/account/100",
            .params = &account_params
        },
        {
            .route = "/account/<account_id>/edit",
            .request = "/account/100/edit",
            .params = &account_params
        },
        {
            .route = "/account/<account_id>/projects",
            .request = "/account/100/projects",
            .params = &account_params
        },
        {
            .route = "/account/<account_id>/contacts",
            .request = "/account/100/contacts",
            .params = &account_params
        },
        {
            .route = "/account/<account_id>/contact/<account_contact_id>",
            .request = "/account/200/contact/300",
            .params = &account_contact_params
        },
        {
            .route = "/projects",
            .request = "/projects",
            .params = NULL
        },
        {
            .route = "/projects/create",
            .request = "/projects/create",
            .params = NULL
        },
        {
            .route = "/project/<project_id>",
            .request = "/project/400",
            .params = &project_params
        },
        {
            .route = "/project/<project_id>/edit",
            .request = "/project/400/edit",
            .params = &project_params
        },
    };

    wrouter_param_syntax_t param_syntax = WROUTER_SYNTAX_ANGLE;
    wrouter_builder_t *builder = wrouter_builder_create(param_syntax);


    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        //wrouter_add_route(builder, pattern, route);
    }

    uint32_t status = 0;
    wrouter_t *router = wrouter_compile(builder, &status);

    wrouter_builder_free(builder);

    wrouter_free(router);
}

int main(void)
{
    test_router_basic();
    return 0;
}
