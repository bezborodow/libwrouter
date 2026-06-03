#include "token.h"
#include "graph.h"
#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "prelexer.h"
#include "symbol.h"
#include "segment.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

static inline void builder_retain(const wrouter_builder_t *builder, const wrouter_route_t *route)
{
    if (builder->retain != NULL)
        builder->retain(route->ctx);
}

/**
 * Create a route builder.
 *
 * Use the builder to create a router tree by adding routes to it. Then compile
 * the tree into a router graph. After this, free the builder.
 *
 * The builder is not thread-safe.
 */
wrouter_builder_t *wrouter_builder_create(const wrouter_options_t options)
{
    wrouter_builder_t *builder;

    builder = calloc(1, sizeof(*builder));
    if (builder == NULL)
        return NULL;

    builder->param_syntax = options.param_syntax;
    builder->fallback.handler = options.fallback_handler;
    builder->fallback.ctx = options.fallback_ctx;
    builder->retain = options.retain;
    builder->release = options.release;

    builder_retain(builder, &builder->fallback);

    builder->root = calloc(1, sizeof(segment_t));
    if (builder->root == NULL)
        goto failure;

    symbol_table_init(&builder->literals);
    symbol_table_init(&builder->params);

    return builder;

failure:
    free(builder);
    return NULL;
}

/**
 * Add a route handler to the route tree.
 */
wrouter_error_t wrouter_add_handler(wrouter_builder_t *builder, const char *pattern,
                                    wrouter_handler_fn handler)
{
    wrouter_route_t route = {
        .handler = handler,
        .ctx = NULL,
    };

    return wrouter_add_route(builder, pattern, route);
}

/**
 * Add a route handler and context to the route tree.
 */
wrouter_error_t wrouter_add_handler_ctx(wrouter_builder_t *builder, const char *pattern,
                                        wrouter_handler_fn handler, const void *ctx)
{
    wrouter_route_t route = {
        .handler = handler,
        .ctx = ctx,
    };

    return wrouter_add_route(builder, pattern, route);
}

wrouter_error_t wrouter_add_context(wrouter_builder_t *builder, const char *pattern,
                                    const void *ctx)
{
    wrouter_route_t route = {
        .handler = NULL,
        .ctx = ctx,
    };

    return wrouter_add_route(builder, pattern, route);
}

static wrouter_route_t *builder_terminate( wrouter_builder_t *builder,
        wrouter_route_t route) {

    wrouter_route_t *terminal = calloc(1, sizeof(*terminal));
    if (terminal == NULL)
        return NULL;

    *terminal = route;

    builder_retain(builder, terminal);

    return terminal;
}

/**
 * Add a route to the route tree.
 */
wrouter_error_t wrouter_add_route(wrouter_builder_t *builder, const char *pattern,
                                  wrouter_route_t route)
{
    token_t tok;
    prelexer_t lx = { 0 };
    prelexer_init(&lx, builder->param_syntax);
    prelexer_load(&lx, pattern);

    segment_t *cur = builder->root;

    for (;;) {
        tok = prelexer_next(&lx);

        switch (tok.type) {
            case TOKEN_END:
                // Check for duplicate routes.
                if (cur->terminal != NULL)
                    return WROUTER_ERR_DUPLICATE_ROUTE;

                // TERMINATE!
                // Append terminal route to the end segment.
                cur->terminal = builder_terminate(builder, route);
                if (cur->terminal == NULL)
                    return WROUTER_ERR_NO_MEMORY;

                return WROUTER_OK;

            case TOKEN_LITERAL: {
                // Literals are incompatible with parameters.
                if (cur->spec_type == SPEC_PARAM)
                    return WROUTER_ERR_LITERAL_CONFLICTS_WITH_PARAM;

                // Check for an existing child.
                segment_t *child = segment_find_child_by_token(cur, tok);

                // If there is not an equivalent literal child of this segment, create one.
                if (child == NULL) {

                    // Append literal symbol to the literal symbol table.
                    if (builder->literals.count >= UINT16_MAX)
                        return WROUTER_ERR_OUT_OF_RANGE;

                    const char *strptr = symbol_append(&builder->literals, tok.ptr, tok.length);
                    if (strptr == NULL)
                        return WROUTER_ERR_NO_MEMORY;

                    // Append child.
                    if (cur->child_count >= NODE_CHILD_MAX)
                        return WROUTER_ERR_OUT_OF_RANGE;

                    segment_t **new_children =
                        realloc(cur->children, sizeof(segment_t *) * (cur->child_count + 1));
                    if (new_children == NULL) // TODO realloc growth.
                        return WROUTER_ERR_NO_MEMORY;

                    cur->children = new_children;

                    child = calloc(1, sizeof(segment_t));
                    if (child == NULL)
                        return WROUTER_ERR_NO_MEMORY;

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
                    return WROUTER_ERR_PARAM_CONFLICTS_WITH_WILDCARD;

                // Parameters are incompatible with literals.
                if (cur->child_count)
                    return WROUTER_ERR_PARAM_CONFLICTS_WITH_LITERAL;

                // Create a parameter on this segment if one does not already exist.
                if (cur->spec_type == SPEC_NONE) {

                    // Append parameter symbol to the parameter symbol table.
                    if (builder->params.count >= UINT16_MAX)
                        return WROUTER_ERR_OUT_OF_RANGE;
                    const char *strptr = symbol_append(&builder->params, tok.ptr, tok.length);
                    if (strptr == NULL)
                        return WROUTER_ERR_NO_MEMORY;

                    segment_t *param = calloc(1, sizeof(segment_t));
                    if (param == NULL)
                        return WROUTER_ERR_NO_MEMORY;

                    param->str = strptr;
                    param->str_length = tok.length;

                    cur->spec_type = SPEC_PARAM;
                    cur->special.param = param;
                } else if (!token_matches_segment(tok, cur->special.param)) {

                    // If a parameter is already assigned, it should have the same name.
                    return WROUTER_ERR_PARAM_NAME_MISMATCH;
                }

                cur = cur->special.param;
                break;
            }

            case TOKEN_WILDCARD: {
                // Check that a wildcard is not already assigned.
                if (cur->spec_type == SPEC_WILDCARD)
                    return WROUTER_ERR_DUPLICATE_ROUTE;

                // Wildcards are incompatible with parameters.
                if (cur->spec_type == SPEC_PARAM)
                    return WROUTER_ERR_WILDCARD_CONFLICTS_WITH_PARAM;

                // Wildcards must be at the end of the pattern string.
                tok = prelexer_next(&lx);
                if (tok.type != TOKEN_END)
                    return WROUTER_ERR_WILDCARD_NOT_FINAL;

                // TERMINATE!
                // Append wildcard terminal route.
                cur->spec_type = SPEC_WILDCARD;
                cur->special.wildcard = builder_terminate(builder, route);
                if (cur->special.wildcard == NULL)
                    return WROUTER_ERR_NO_MEMORY;

                return WROUTER_OK;
            }

            case TOKEN_TRAILING: {
                // Check that a trailing-slash is not already assigned.
                if (cur->trailing != NULL)
                    return WROUTER_ERR_DUPLICATE_ROUTE;

                // TERMINATE!
                // Append trailing-slash terminal route.
                cur->trailing = builder_terminate(builder, route);
                if (cur->trailing == NULL)
                    return WROUTER_ERR_NO_MEMORY;

                return WROUTER_OK;
            }

            case TOKEN_ILLEGAL:
            default:
                return WROUTER_ERR_ILLEGAL_PATTERN;
        }
    }
}

static void builder_release(wrouter_builder_t *builder)
{
    if (builder->release == NULL)
        return;

    // Release all route contexts in the tree.
    segment_release(builder, builder->root);

    // Release fallback route context.
    builder->release(builder->fallback.ctx);
}

/**
 * Free the route tree builder.
 *
 * After compiling the router, there is no need for the builder, and it should
 * therefore be freed to save memory.
 */
void wrouter_builder_free(wrouter_builder_t *builder)
{
    if (builder == NULL)
        return;

    // Release all route contexts.
    builder_release(builder);

    // Free literal and parameter symbol tables.
    symbol_table_free(&builder->literals);
    symbol_table_free(&builder->params);

    // Free all segments.
    segment_free(builder->root);

    // Free the builder.
    free(builder);
}
