#include "graph.h"
#include "token.h"
#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "symbol.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

static int edge_cmp(const void *p1, const void *p2)
{
    const uint16_t *k1 = p1;
    const uint16_t *k2 = p2;

    return (*k1 > *k2) - (*k1 < *k2);
}

/**
 * Calculate graph statistics by traversing the route tree.
 *
 * Amongst other things, this will allow the builder to allocate the exact
 * amount of memory required for building the route graph.
 *
 * @param seg The root segment of the route tree.
 * @param stats Graph statistics.
 */
void graph_stats(const segment_t *seg, graph_stats_t *stats)
{
    stats->nodes++;
    stats->size++;

    // Trailing-slash edge.
    if (seg->trailing != NULL)
        stats->size++;

    // Special edges.
    switch (seg->spec_type) {
        case SPEC_WILDCARD:
            stats->size++;
        case SPEC_PARAM:
            stats->size += 2;
            break;

        case SPEC_NONE:
            break;
    }

    // Literal child node edges and nodes.
    stats->symbolic_edges += seg->child_count;
    stats->size += 3 * seg->child_count;

    for (segment_t *child = seg->head; child; child = child->next)
        graph_stats(child, stats);

    // Special nodes.
    switch (seg->spec_type) {
        case SPEC_PARAM:
            stats->edges++;
            stats->param_depth++;
            if (stats->param_depth > stats->max_params)
                stats->max_params++;
            graph_stats(seg->special.param, stats);
            stats->param_depth--;
            break;

        // Wildcard node.
        // A wildcard requires an edge, a node, and always terminates.
        case SPEC_WILDCARD:
            stats->edges++;
            stats->nodes++;
            stats->terminals++;

            // Reserve space for wildcard parameter.
            if (stats->param_depth >= stats->max_params)
                stats->max_params++;

            stats->size++;
            break;

        case SPEC_NONE:
            break;
    }

    // Trailing-slash node.
    // A trailing-slash requires an edge, a node, and always terminates.
    if (seg->trailing != NULL) {
        stats->edges++;
        stats->nodes++;
        stats->terminals++;
        stats->size++;
    }

    // Terminal node.
    if (seg->terminal != NULL)
        stats->terminals++;
}

uint16_t *graph_compile(wrouter_t *router, const segment_t *segment, uint16_t *cursor)
{
    uint16_t *g = router->graph;
    uint16_t *node = NULL, *l_node = NULL;
    uint16_t *l_edge_base = NULL, *p_edge = NULL, *w_edge = NULL, *t_edge = NULL;
    uint16_t *l_sym = NULL, *p_sym = NULL;

    // Append node.
    node = cursor++;

    // Store number of literals.
    *node |= segment->child_count;

    // Terminate node.
    if (segment->terminal != NULL) {

        // Store termination flag.
        *node |= NODE_FLAG_TERMINAL;

        // Copy routes into the terminal dictionary.
        //
        // refs is the terminating node's graph offset. Searching for the
        // offset yields an index that is used to lookup the terminal in the
        // base array.
        terminal_append(&router->terminals, (ptrdiff_t)(node - g), *segment->terminal);

        // Retain context.
        // Callback to retain context reference count for garbage collection if
        // required (for example, the Python library needs this.)
        router_retain(router, segment->terminal);
    }

    // Special edges.
    switch (segment->spec_type) {
        // Parameter edge.
        case SPEC_PARAM:
            *node |= NODE_FLAG_HAS_PARAM;
            p_sym = cursor++;
            p_edge = cursor++;
            *p_sym = symbol_resolve(&router->params, segment->special.param->str);
            break;

        // Wildcard edge.
        case SPEC_WILDCARD:
            *node |= NODE_FLAG_HAS_WILDCARD;
            w_edge = cursor++;
            break;

        case SPEC_NONE:
            break;
    }

    // Trailing-slash edge is stored after the special edge if one exists.
    if (segment->trailing != NULL) {
        *node |= NODE_FLAG_HAS_TRAILING;
        t_edge = cursor++;
    }

    // Descend into literals.
    if (segment->child_count) {
        // Find the start address for literal edges.
        uint16_t *l_edge_base = cursor;

        // Resolve symbols and save into into the literal edges.
        for (segment_t *child = segment->head; child; child = child->next) {
            l_sym = cursor++;
            cursor++;
            *l_sym = symbol_resolve(&router->literals, child->str);
        }

        // Recurse into literal nodes and save their offsets.
        uint16_t i = 0;
        for (segment_t *child = segment->head; child; child = child->next) {
            l_node = graph_compile(router, child, cursor);
            *(l_edge_base + i * 2 + 1) = (ptrdiff_t)(l_node - g);
        }

        // Sort the edges by symbol.
        qsort(l_edge_base, segment->child_count * 2, sizeof(uint16_t), edge_cmp);
    }

    // Descend into parameter.
    if (p_edge != NULL) {
        uint16_t *p_node = graph_compile(router, segment->special.param, cursor);
        *p_edge = p_node - g;
    }

    // Append wildcard node.
    if (w_edge != NULL) {
        uint16_t *w_node = cursor++;
        *w_node |= NODE_FLAG_TERMINAL;
        *w_edge = (ptrdiff_t)(w_node - g);
        terminal_append(&router->terminals, (ptrdiff_t)(w_edge - g), *segment->special.wildcard);
        router_retain(router, segment->special.wildcard);
    }

    // Append trailing node.
    if (t_edge != NULL) {
        uint16_t *t_node = cursor++;
        *t_node |= NODE_FLAG_TERMINAL;
        *t_edge = (ptrdiff_t)(t_node - g);
        terminal_append(&router->terminals, (ptrdiff_t)(t_edge - g), *segment->trailing);
        router_retain(router, segment->trailing);
    }

    return node;
}
