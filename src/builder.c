#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "prelexer.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

wrouter_builder_t *wrouter_builder_create(wrouter_param_syntax_t param_syntax)
{
    wrouter_builder_t *builder;

    builder = calloc(1, sizeof(*builder));

    if (builder == NULL)
        return NULL;

    builder->param_syntax = param_syntax;

    symbol_table_init(&builder->literals);
    symbol_table_init(&builder->params);

    return builder;
}

int wrouter_add_route(wrouter_builder_t *builder, const char *pattern, wrouter_route_t route)
{
    int status = 0;

    pretoken_t tok;
    prelexer_t lx = { 0 };
    prelexer_init(&lx, builder->param_syntax);
    prelexer_load(&lx, pattern);

    for (;;) {
        tok = prelexer_next(&lx);

        if (tok.type == TOKEN_END)
            break;

        if (tok.type == TOKEN_ILLEGAL)
            return -1;

        if (tok.type == TOKEN_LITERAL) {
            status = symbol_append(&builder->literals, tok.ptr, tok.length);
            if (status)
                return status;
        }

        if (tok.type == TOKEN_PARAM) {
            status = symbol_append(&builder->params, tok.ptr, tok.length);
            if (status)
                return status;
        }
    }

    return 0;
}

static int strpcmp(const void *p1, const void *p2)
{
    return strcmp(*(const char **)p1, *(const char **)p2);
}

wrouter_t *wrouter_compile(const wrouter_builder_t *builder)
{
    wrouter_t *router = malloc(sizeof(struct router));
    if (router == NULL)
        return NULL;

    qsort(builder->literals.base, builder->literals.count, sizeof(char *), strpcmp);
    qsort(builder->params.base, builder->params.count, sizeof(char *), strpcmp);

    return router;
}

void wrouter_builder_free(wrouter_builder_t *builder)
{
    if (builder == NULL)
        return;

    symbol_table_free(&builder->literals);
    symbol_table_free(&builder->params);

    free(builder);
}
