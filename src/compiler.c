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

/**
 * Compile the route graph from the builder's route tree.
 *
 * This will compile an immutable router from a route tree, which is therefore
 * thread-safe. The router consists of a graph, symbols, and terminals.
 */
wrouter_t *wrouter_compile(const wrouter_builder_t *builder, wrouter_error_t *err)
{
    graph_stats_t stats = { 0 };
    size_t cursor = 0;

    *err = WROUTER_OK;

    if (builder->corrupted) {
        *err = WROUTER_ERR_BUILDER_CORRUPTED;
        return NULL;
    }

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

/**
 * Compile the router and destroy the builder.
 */
wrouter_t *wrouter_consume(wrouter_builder_t **bpp, wrouter_error_t *err)
{
    if (err == NULL)
        return NULL;

    if (bpp == NULL || *bpp == NULL) {
        if (err)
            *err = WROUTER_ERR_NULL_ARGUMENT;

        return NULL;
    }

    wrouter_t *router = wrouter_compile(*bpp, err);

    wrouter_builder_destroy(bpp);

    return router;
}
