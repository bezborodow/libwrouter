#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "prelexer.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

struct builder *wrouter_builder_create(wrouter_param_syntax_t param_syntax)
{
    struct builder *builder;

    builder = calloc(1, sizeof(*builder));
    if (builder == NULL)
        return NULL;

    builder->param_syntax = param_syntax;

    builder->root = calloc(1, sizeof(segment_t));
    if (builder->root == NULL) {
        free(builder);
        return NULL;
    }

    symbol_table_init(&builder->literals);
    symbol_table_init(&builder->params);

    return builder;
}

static bool token_matches(pretoken_t tok, const segment_t *seg)
{
    return seg->str && tok.ptr && tok.length == seg->str_length &&
           strncmp(tok.ptr, seg->str, tok.length) == 0;
}

static segment_t *find_child(segment_t *segment, pretoken_t tok)
{
    if (tok.ptr == NULL)
        return NULL;

    for (uint16_t i = 0; i < segment->child_count; i++) {
        segment_t *child = segment->children[i];

        if (token_matches(tok, child))
            return child;
    }

    return NULL;
}

int wrouter_add_route(struct builder *builder, const char *pattern, struct route route)
{
    if (route.handler == NULL)
        return -1;

    pretoken_t tok;
    prelexer_t lx = { 0 };
    prelexer_init(&lx, builder->param_syntax);
    prelexer_load(&lx, pattern);

    segment_t *cur = builder->root;

    for (;;) {
        tok = prelexer_next(&lx);

        switch (tok.type) {
            case TOKEN_END:
                // Check for duplicate routes.
                if (cur->route.handler != NULL)
                    return -1;

                // Terminate route.
                cur->route = route;
                return 0;

            case TOKEN_LITERAL: {
                // Literals are incompatible with parameters.
                if (cur->spec_type == SPEC_PARAM)
                    return -1;

                // Check for an existing child.
                segment_t *child = find_child(cur, tok);

                // If not, create one.
                if (child == NULL) {

                    const char *strptr = symbol_append(&builder->literals, tok.ptr, tok.length);
                    if (strptr == NULL)
                        return -1;

                    // Append child.
                    segment_t **new_children =
                        realloc(cur->children, sizeof(segment_t *) * (cur->child_count + 1));
                    if (new_children == NULL)
                        return -1;
                    cur->children = new_children;

                    child = calloc(1, sizeof(segment_t));
                    if (child == NULL)
                        return -1;

                    child->str = strptr;
                    child->str_length = tok.length;
                    cur->children[cur->child_count++] = child;
                }

                cur = child;
                break;
            }

            case TOKEN_PARAM: {
                // Parameters are incompatible with wildcards.
                if (cur->spec_type == SPEC_WILDCARD)
                    return -1;

                if (cur->spec_type == SPEC_PARAM) {

                    // If a parameter is already assigned, it should have the same name.
                    if (!token_matches(tok, cur->special.param))
                        return -1;

                } else {

                    // Append parameter.
                    const char *strptr = symbol_append(&builder->params, tok.ptr, tok.length);
                    if (strptr == NULL)
                        return -1;

                    segment_t *param = calloc(1, sizeof(segment_t));
                    if (param == NULL)
                        return -1;

                    param->str = strptr;
                    param->str_length = tok.length;

                    cur->spec_type = SPEC_PARAM;
                    cur->special.param = param;
                }

                cur = cur->special.param;
                break;
            }
            case TOKEN_WILDCARD: {
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
            }
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

        case SPEC_NONE:
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
