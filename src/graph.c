#include "graph.h"
#include "token.h"
#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "symbol.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

static int edge_cmp(const void *p1, const void *p2)
{
    const edge_t *e1 = p1;
    const edge_t *e2 = p2;

    return e1->symbol - e2->symbol;
}

/**
 * Align the cursor to the next memory location for a given alignment.
 */
static inline uintptr_t align_up(size_t cursor, size_t align)
{
    return (cursor + align - 1) & ~(align - 1);
}

/**
 * Add to the summation of the total size of memory required by graph edges and
 * nodes with consideration for alignment.
 */
static void size_up(size_t *total_size, size_t align, size_t size)
{
    if (!size)
        return;

    *total_size = align_up(*total_size, align);
    *total_size += size;
}

size_t graph_offset(const void *graph, const void *entry)
{
    return (const uint8_t *)entry - (const uint8_t *)graph;
}

inline const edge_t *node_edge_base(const node_t *node)
{
    uintptr_t align = _Alignof(edge_t);
    uintptr_t cursor = (uintptr_t)node + sizeof(node_t);
    uintptr_t base = (cursor + align - 1) & ~(align - 1);

    return (const edge_t *)base;
}

inline const node_t *next_node(const uint8_t *graph, const edge_t *edge)
{
    return (const node_t *)(graph + edge->next);
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
    size_up(&stats->size, _Alignof(node_t), sizeof(node_t));

    // Trailing-slash edge.
    if (seg->trailing != NULL)
        size_up(&stats->size, _Alignof(edge_t), sizeof(edge_t));

    // Special edges.
    switch (seg->spec_type) {
        case SPEC_PARAM:
        case SPEC_WILDCARD:
            size_up(&stats->size, _Alignof(edge_t), sizeof(edge_t));
            break;

        case SPEC_NONE:
            break;
    }

    // Literal child node edges and nodes.
    stats->symbolic_edges += seg->child_count;
    size_up(&stats->size, _Alignof(edge_t), seg->child_count * sizeof(edge_t));
    for (uint16_t i = 0; i < seg->child_count; i++) {
        segment_t *child = seg->children[i];
        graph_stats(child, stats);
    }

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

            size_up(&stats->size, _Alignof(node_t), sizeof(node_t));
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
        size_up(&stats->size, _Alignof(node_t), sizeof(node_t));
    }

    // Terminal node.
    if (seg->terminal != NULL)
        stats->terminals++;
}

/**
 * Increases the cursor and returns the base.
 */
static void *graph_append(void *g, size_t *cursor, size_t size, size_t align)
{
    if (!size)
        return NULL;

    *cursor = align_up(*cursor, align);

    void *base = (uint8_t *)g + *cursor;
    memset(base, 0, size);
    *cursor += size;

    return base;
}

static node_t *graph_append_node(void *g, size_t *cursor)
{
    return graph_append(g, cursor, sizeof(node_t), _Alignof(node_t));
}

static edge_t *graph_append_edge(void *g, size_t *cursor)
{
    return graph_append(g, cursor, sizeof(edge_t), _Alignof(edge_t));
}

static edge_t *graph_append_edges(void *g, size_t *cursor, size_t nmemb)
{
    return graph_append(g, cursor, nmemb * sizeof(edge_t), _Alignof(edge_t));
}

node_t *graph_compile(wrouter_t *router, const segment_t *segment, size_t *cursor)
{
    void *g = router->graph;
    node_t *node = NULL;
    edge_t *p_edge = NULL, *w_edge = NULL, *t_edge = NULL;

    // Append node.
    node = graph_append_node(g, cursor);

    // Store number of literals.
    node->literals = segment->child_count;

    // Terminate node.
    if (segment->terminal != NULL) {

        // Store termination flag.
        node->flags |= NODE_FLAG_TERMINAL;

        // Copy routes into the terminal dictionary.
        //
        // refs is the terminating node's graph offset. Searching for the
        // offset yields an index that is used to lookup the terminal in the
        // base array.
        router->terminals.refs[router->terminals.count] = graph_offset(g, node);
        router->terminals.base[router->terminals.count++] = *segment->terminal;

        // Retain context.
        // Callback to retain context reference count for garbage collection if
        // required (for example, the Python library needs this.)
        router_retain(router, segment->terminal);
    }

    // Special edges.
    switch (segment->spec_type) {
        // Parameter edge.
        case SPEC_PARAM:
            node->flags |= NODE_FLAG_HAS_PARAM;
            p_edge = graph_append_edge(g, cursor);
            p_edge->symbol = symbol_resolve(segment->special.param->str, router->params.base,
                                            router->params.count);
            break;

        // Wildcard edge.
        case SPEC_WILDCARD:
            node->flags |= NODE_FLAG_HAS_WILDCARD;
            w_edge = graph_append_edge(g, cursor);
            break;

        case SPEC_NONE:
            break;
    }

    // Trailing-slash edge is stored after the special edge if one exists.
    if (segment->trailing != NULL) {
        node->flags |= NODE_FLAG_HAS_TRAILING;
        t_edge = graph_append_edge(g, cursor);
    }

    // Descend into literals.
    if (segment->child_count) {
        // Find the start address for literal edges.
        edge_t *l_edge_base = graph_append_edges(g, cursor, segment->child_count);

        // Resolve symbols and save into into the literal edges.
        for (uint16_t i = 0; i < segment->child_count; i++) {
            segment_t *child = segment->children[i];
            edge_t *l_edge = &l_edge_base[i];
            l_edge->symbol =
                symbol_resolve(child->str, router->literals.base, router->literals.count);
        }

        // Recurse into literal nodes and save their offsets.
        for (uint16_t i = 0; i < segment->child_count; i++) {
            segment_t *child = segment->children[i];

            node_t *l_node = graph_compile(router, child, cursor);

            edge_t *l_edge = &l_edge_base[i];
            l_edge->next = graph_offset(g, l_node);
        }

        // Sort the edges by symbol.
        qsort(l_edge_base, segment->child_count, sizeof(edge_t), edge_cmp);
    }

    // Descend into parameter.
    if (p_edge != NULL) {
        node_t *p_node = graph_compile(router, segment->special.param, cursor);
        p_edge->next = graph_offset(g, p_node);
    }

    // Append wildcard node.
    if (w_edge != NULL) {
        node_t *w_node = graph_append_node(g, cursor);
        w_node->flags |= NODE_FLAG_TERMINAL;
        w_edge->next = graph_offset(g, w_node);
        router->terminals.refs[router->terminals.count] = w_edge->next;
        router->terminals.base[router->terminals.count++] = *segment->special.wildcard;

        // Retain context.
        router_retain(router, segment->special.wildcard);
    }

    // Append trailing node.
    if (t_edge != NULL) {
        node_t *t_node = graph_append_node(g, cursor);
        t_node->flags |= NODE_FLAG_TERMINAL;
        t_edge->next = graph_offset(g, t_node);
        router->terminals.refs[router->terminals.count] = t_edge->next;
        router->terminals.base[router->terminals.count++] = *segment->trailing;

        // Retain context.
        router_retain(router, segment->trailing);
    }

    return node;
}
