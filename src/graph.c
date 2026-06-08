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

#include <stdio.h>

static int edge_cmp(const void *a, const void *b)
{
    const uint16_t *ea = a;
    const uint16_t *eb = b;

    if (ea[0] < eb[0])
        return -1;
    if (ea[0] > eb[0])
        return 1;
    return 0;
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
    // This node.
    stats->size++;
    if (seg->terminal != NULL)
        stats->terminals++;

    // Trailing-slash.
    // 1 edge, 1 node.
    if (seg->trailing != NULL) {
        stats->terminals++;
        stats->size += 2;
    }

    // Special edges.
    switch (seg->spec_type) {
        case SPEC_WILDCARD:
            // 1 edge, 1 node.
            stats->size += 2;
            stats->terminals++;

            // Reserve space for wildcard parameter.
            if (stats->param_depth >= stats->max_params)
                stats->max_params++;

            break;

        case SPEC_PARAM:
            // 2 edge, nodes descend.
            stats->size += 2;
            stats->param_depth++;
            if (stats->param_depth > stats->max_params)
                stats->max_params++;
            graph_stats(seg->special.param, stats);
            stats->param_depth--;
            break;

        case SPEC_NONE:
            break;
    }

    // Two edges per literal child, descend nodes.
    stats->size += 2 * seg->child_count;

    for (segment_t *child = seg->head; child; child = child->next)
        graph_stats(child, stats);
}

uint16_t *graph_compile(wrouter_t *router, const segment_t *segment, uint16_t **pcur)
{
    uint16_t *g = router->graph;
    uint16_t *node = NULL, *l_node = NULL;
    uint16_t *l_edge_base = NULL, *p_edge = NULL, *w_edge = NULL, *t_edge = NULL;
    uint16_t *l_sym = NULL, *p_sym = NULL;

    fprintf(stderr, "Cursor: %u\n", (ptrdiff_t)(*pcur - g));

    // Append node.
    node = (*pcur)++;

    // Store number of literals.
    *node |= segment->child_count;

    fprintf(stderr, "Children: %u\n", segment->child_count);

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
            p_sym = (*pcur)++;
            p_edge = (*pcur)++;
            *p_sym = symbol_resolve(&router->params, segment->special.param->str);
            break;

        // Wildcard edge.
        case SPEC_WILDCARD:
            *node |= NODE_FLAG_HAS_WILDCARD;
            w_edge = (*pcur)++;
            break;

        case SPEC_NONE:
            break;
    }

    // Trailing-slash edge is stored after the special edge if one exists.
    if (segment->trailing != NULL) {
        *node |= NODE_FLAG_HAS_TRAILING;
        t_edge = (*pcur)++;
    }

    // Descend into literals.
    if (segment->child_count) {
        // Find the start address for literal edges.
        uint16_t *l_edge_base = *pcur;
        fprintf(stderr, "Has children\n");

        // Resolve symbols and save into into the literal edges.
        for (segment_t *child = segment->head; child; child = child->next) {
            l_sym = (*pcur)++;
            (*pcur)++;
            *l_sym = symbol_resolve(&router->literals, child->str);
            fprintf(stderr, "Symbol: %u; cursor: %u\n", *l_sym, (ptrdiff_t)(*pcur - g));
        }

        fprintf(stderr, "done literal edges %u\n", (ptrdiff_t)(*pcur - g));

        // Recurse into literal nodes and save their offsets.
        uint16_t i = 0;
        for (segment_t *child = segment->head; child; child = child->next) {
            fprintf(stderr, "START DESCEND NODE ADDR: %u EDGE: %u\n", (ptrdiff_t)(l_node - g), (ptrdiff_t)(l_edge_base + i * 2 + 1 - g));
            l_node = graph_compile(router, child, pcur);
            *(l_edge_base + i * 2 + 1) = (ptrdiff_t)(l_node - g);
            //*(l_edge_base + i * 2 + 1) = 500;
            fprintf(stderr, "END DESCEND NODE ADDR: %u EDGE: %u\n", (ptrdiff_t)(l_node - g), (ptrdiff_t)(l_edge_base + i * 2 + 1 - g));
            i++;
        }

        // Sort the edges by symbol.
        // TODO SORT EDGES DOES NOT WORK TODO TODO TODO FIXME
        qsort(l_edge_base, segment->child_count, 2 * sizeof(uint16_t), edge_cmp);
    }

    // Descend into parameter.
    if (p_edge != NULL) {
        uint16_t *p_node = graph_compile(router, segment->special.param, pcur);
        *p_edge = (ptrdiff_t)(p_node - g);
    }

    // Append wildcard node.
    if (w_edge != NULL) {
        uint16_t *w_node = (*pcur)++;
        *w_node |= NODE_FLAG_TERMINAL;
        *w_edge = (ptrdiff_t)(w_node - g);
        terminal_append(&router->terminals, (ptrdiff_t)(w_edge - g), *segment->special.wildcard);
        router_retain(router, segment->special.wildcard);
    }

    // Append trailing node.
    if (t_edge != NULL) {
        uint16_t *t_node = (*pcur)++;
        *t_node |= NODE_FLAG_TERMINAL;
        *t_edge = (ptrdiff_t)(t_node - g);
        terminal_append(&router->terminals, (ptrdiff_t)(t_edge - g), *segment->trailing);
        router_retain(router, segment->trailing);
    }

    return node;
}
