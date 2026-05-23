#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "prelexer.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct builder *wrouter_builder_create(wrouter_param_syntax_t param_syntax)
{
    struct builder *builder;

    builder = calloc(1, sizeof(*builder));

    if (builder == NULL)
        return NULL;

    builder->param_syntax = param_syntax;

    symbol_table_init(&builder->literals);
    symbol_table_init(&builder->params);
    builder->root = calloc(1, sizeof(segment_t));

    return builder;
}

int wrouter_add_route(struct builder *builder, const char *pattern, struct route route)
{
    pretoken_t tok;
    prelexer_t lx = { 0 };
    prelexer_init(&lx, builder->param_syntax);
    prelexer_load(&lx, pattern);

    segment_t *cur = builder->root;
    char *strptr;

    for (uint8_t depth = 0; ; depth++) {
        tok = prelexer_next(&lx);

        switch (tok.type) {
            case TOKEN_END:
                // Terminate route.
                cur->route = route;
                return 0;

            case TOKEN_LITERAL:
                strptr = symbol_append(&builder->literals, tok.ptr, tok.length);
                if (strptr == NULL)
                    return -1;

                segment_t **new_children = realloc(cur->children,
                        sizeof(segment_t*) * (cur->child_count + 1));

                if (new_children == NULL)
                    return -1;

                cur->children = new_children;
                segment_t *child = calloc(1, sizeof(segment_t));
                if (child == NULL)
                    return -1;

                child->str = strptr;
                child->str_length = tok.length;
                cur->children[cur->child_count++] = child;

                cur = child;
                break;

            case TOKEN_PARAM:
                // Check that a parameter is not already assigned.
                if (cur->spec_type == SPEC_PARAM)
                    return -1;

                // Parameters are incompatible with wildcards.
                if (cur->spec_type == SPEC_WILDCARD)
                    return -1;

                // Append parameter.
                strptr = symbol_append(&builder->params, tok.ptr, tok.length);
                if (strptr == NULL)
                    return -1;

                segment_t *param = calloc(1, sizeof(segment_t));
                if (param == NULL)
                    return -1;

                param->str = strptr;
                param->str_length = tok.length;

                cur->spec_type = SPEC_PARAM;
                cur->special.param = param;
                break;

            case TOKEN_WILDCARD:
                // Check that a wildcard is not already assigned.
                if (cur->spec_type == SPEC_WILDCARD)
                    return -1;

                // Wildcards are incompatible with parameters.
                if (cur->spec_type == SPEC_PARAM)
                    return -1;

                // Wildcards must be terminal.
                tok = prelexer_next(&lx);
                if (tok.type != TOKEN_END)
                    return -1;

                // Append wildcard.
                cur->special.wildcard = calloc(1, sizeof(wildcard_t));
                if (cur->special.wildcard == NULL)
                    return -1;
                cur->spec_type = SPEC_WILDCARD;
                cur->special.wildcard->route = route;
                return 0;

            case TOKEN_ILLEGAL:
            default:
                return -1;
        }
    }

    return 0;
}

static int strpcmp(const void *p1, const void *p2)
{
    return strcmp(*(const char **)p1, *(const char **)p2);
}

struct router *wrouter_compile(const struct builder *builder)
{
    wrouter_t *router = calloc(1, sizeof(struct router));
    if (router == NULL)
        return NULL;

    qsort(builder->literals.base, builder->literals.count, sizeof(char *), strpcmp);
    qsort(builder->params.base, builder->params.count, sizeof(char *), strpcmp);

    return router;
}

static void segment_free(segment_t *segment)
{
    if (segment == NULL)
        return;

    switch (segment->spec_type) {
        case SPEC_WILDCARD:
            free(segment->special.wildcard);
            break;

        case SPEC_PARAM:
            segment_free(segment->special.param);
            break;
    }

    for (uint16_t i = 0; i < segment->child_count; i++)
        segment_free(segment->children[i]);

    free(segment->children);
    free(segment);
}

void wrouter_builder_free(struct builder *builder)
{
    if (builder == NULL)
        return;

    symbol_table_free(&builder->literals);
    symbol_table_free(&builder->params);

    segment_free(builder->root);
    free(builder);
}
