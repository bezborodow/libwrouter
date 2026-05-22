#include "wrouter.h"
#include "builder.h"
#include "prelexer.h"
#include <stdlib.h>

wrouter_builder_t *wrouter_builder_create(wrouter_param_syntax_t param_syntax)
{
    wrouter_builder_t *builder;

    builder = calloc(1, sizeof(*builder));

    if (builder == NULL)
        return NULL;

    builder->param_syntax = param_syntax;

    symbol_table_init(&builder->symctx.literals);
    symbol_table_init(&builder->symctx.params);

    return builder;
}

int wrouter_add_route(wrouter_builder_t *builder, const char *pattern, wrouter_handler_t handler,
                      void *handler_ctx)
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
            status = symbol_append(&builder->symctx.literals, tok.ptr, tok.length);
            if (status)
                return status;
        }

        if (tok.type == TOKEN_PARAM) {
            status = symbol_append(&builder->symctx.params, tok.ptr, tok.length);
            if (status)
                return status;
        }
    }

    return 0;
}

int wrouter_compile(const wrouter_builder_t *builder, wrouter_t **router)
{

}

void wrouter_builder_free(wrouter_builder_t *builder)
{
    if (builder == NULL)
        return;

    symbol_table_free(&builder->symctx.literals);
    symbol_table_free(&builder->symctx.params);

    free(builder);
}
