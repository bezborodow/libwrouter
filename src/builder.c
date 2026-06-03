#include "token.h"
#include "graph.h"
#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "prelexer.h"
#include "symbol.h"
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
 * Find a child of a segment by token.
 */
static segment_t *find_child_by_token(segment_t *segment, token_t tok)
{
    if (tok.ptr == NULL)
        return NULL;

    for (uint16_t i = 0; i < segment->child_count; i++) {
        segment_t *child = segment->children[i];

        if (token_matches_segment(tok, child))
            return child;
    }

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
                if (cur->terminal)
                    return WROUTER_ERR_DUPLICATE_ROUTE;

                // TERMINATE!
                // Append terminal route to the end segment.
                cur->route = route;
                cur->terminal = true;
                builder_retain(builder, &route);

                return WROUTER_OK;

            case TOKEN_LITERAL: {
                // Literals are incompatible with parameters.
                if (cur->spec_type == SPEC_PARAM)
                    return WROUTER_ERR_LITERAL_CONFLICTS_WITH_PARAM;

                // Check for an existing child.
                segment_t *child = find_child_by_token(cur, tok);

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

                // Check if a parmeter already exists on this segment, otherwise create one.
                if (cur->spec_type == SPEC_PARAM) {

                    // If a parameter is already assigned, it should have the same name.
                    if (!token_matches_segment(tok, cur->special.param))
                        return WROUTER_ERR_PARAM_NAME_MISMATCH;

                } else {

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
                cur->special.wildcard = calloc(1, sizeof(wildcard_t));
                if (cur->special.wildcard == NULL)
                    return WROUTER_ERR_NO_MEMORY;
                cur->spec_type = SPEC_WILDCARD;
                cur->special.wildcard->route = route;

                builder_retain(builder, &route);

                return WROUTER_OK;
            }

            case TOKEN_TRAILING: {
                // Check that a trailing-slash is not already assigned.
                if (cur->trailing)
                    return WROUTER_ERR_DUPLICATE_ROUTE;

                // TERMINATE!
                // Append trailing-slash terminal route.
                cur->trailing = calloc(1, sizeof(trailing_t));
                if (cur->trailing == NULL)
                    return WROUTER_ERR_NO_MEMORY;
                cur->trailing->route = route;

                builder_retain(builder, &route);

                return WROUTER_OK;
            }

            case TOKEN_ILLEGAL:
            default:
                return WROUTER_ERR_ILLEGAL_PATTERN;
        }
    }
}

/**
 * Compile the route tree.
 *
 * This will compile an immutable router from a route tree, which is therefore
 * thread-safe. The router consists of a graph, symbols, and terminals.
 */
wrouter_t *wrouter_compile(const wrouter_builder_t *builder, wrouter_error_t *err)
{
    graph_stats_t stats = { 0 };
    size_t cursor = 0;

    *err = WROUTER_OK;

    // New router.
    wrouter_t *router = calloc(1, sizeof(wrouter_t));
    if (router == NULL)
        return NULL;

    // Copy options from the builder onto the router.
    router->fallback = builder->fallback;
    router->retain = builder->retain;
    router->release = builder->release;
    router_retain(router, &router->fallback);

    // Obtain graph statistics.
    graph_stats(builder->root, &stats);
    router->num_routes = stats.terminals;
    router->max_params = stats.max_params;

    // If no terminals, return an empty router.
    if (!stats.terminals)
        return router;

    // Range checking.
    if (stats.size > UINT16_MAX || stats.terminals > UINT16_MAX)
        goto out_of_range;

    // Allocate and compile symbols for literals.
    if ((*err = symbol_compile(&builder->literals, &router->literals)))
        goto failure;

    // Allocate and compile symbols for parameters.
    if ((*err = symbol_compile(&builder->params, &router->params)))
        goto failure;

    // Allocate terminal refs.
    router->terminals.refs = calloc(stats.terminals, sizeof(uint16_t));
    if (router->terminals.refs == NULL)
        goto no_memory;

    // Allocate terminals.
    router->terminals.base = calloc(stats.terminals, sizeof(wrouter_route_t));
    if (router->terminals.base == NULL)
        goto no_memory;

    // Allocate the graph.
    router->graph = malloc(stats.size);
    if (router->graph == NULL)
        goto no_memory;

    // Compile the graph.
    graph_compile(router, builder->root, &cursor);

    return router;

out_of_range:
    *err = WROUTER_ERR_OUT_OF_RANGE;
    goto failure;

no_memory:
    *err = WROUTER_ERR_NO_MEMORY;
    goto failure;

failure:
    wrouter_free(router);
    return NULL;
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

    if (segment->trailing)
        free(segment->trailing);

    for (uint16_t i = 0; i < segment->child_count; i++)
        segment_free(segment->children[i]);

    free(segment->children);
    free(segment);
}

static void segment_release(wrouter_builder_t *builder, const segment_t *seg)
{
    // Descend into literals.
    for (uint16_t i = 0; i < seg->child_count; i++) {
        segment_release(builder, seg->children[i]);
    }

    switch (seg->spec_type) {
        case SPEC_PARAM:
            // Descend into parameters.
            segment_release(builder, seg->special.param);
            break;

        case SPEC_WILDCARD:
            // Release wildcard route context.
            builder->release(seg->special.wildcard->route.ctx);
            break;

        case SPEC_NONE:
            break;
    }

    // Release trailing-slash route context.
    if (seg->trailing)
        builder->release(seg->trailing->route.ctx);

    // Release segment terminal route context.
    if (seg->terminal)
        builder->release(seg->route.ctx);
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
