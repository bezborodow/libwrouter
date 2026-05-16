#include "wrouter.h"
#include "builder.h"

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
