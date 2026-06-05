#include "graph.h"
#include "wrouter.h"
#include "router.h"
#include "symbol.h"
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

    // Require an error output so failures can be reported.
    if (err == NULL)
        return NULL;

    *err = WROUTER_OK;

    if (builder == NULL) {
        *err = WROUTER_ERR_NULL_ARGUMENT;
        return NULL;
    }

    if (builder->corrupted) {
        *err = WROUTER_ERR_BUILDER_CORRUPTED;
        return NULL;
    }

    // New router.
    wrouter_t *router = calloc(1, sizeof(wrouter_t));
    if (router == NULL)
        goto no_memory;

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
    if (stats.size > GRAPH_CAPACITY_BYTES || stats.terminals > UINT16_MAX)
        goto out_of_range;

    // Allocate and compile symbols for literals.
    if ((*err = symbol_compile(&builder->literals, &router->literals)))
        goto failure;

    // Allocate and compile symbols for parameters.
    if ((*err = symbol_compile(&builder->params, &router->params)))
        goto failure;

    if (terminal_alloc(&router->terminals, stats.terminals))
        goto no_memory;

    // Allocate the graph.
    router->graph = calloc(stats.size, 1);
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
    wrouter_t *router;

    if (err == NULL)
        goto failure;

    if (bpp == NULL || *bpp == NULL) {
        *err = WROUTER_ERR_NULL_ARGUMENT;
        goto failure;
    }

    router = wrouter_compile(*bpp, err);

    wrouter_builder_destroy(bpp);

    return router;

failure:

    // Always destroy the builder.
    wrouter_builder_destroy(bpp);
    return NULL;
}
