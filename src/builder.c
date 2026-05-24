#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "prelexer.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

static inline size_t align_up(size_t cursor, size_t align)
{
    return (cursor + align - 1) & ~(align - 1);
}

static void size_up(size_t *total_size, size_t align, size_t size)
{
    if (!size)
        return;

    *total_size = align_up(*total_size, align);
    *total_size += size;
}

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

static bool token_matches(token_t tok, const segment_t *seg)
{
    return seg->str && tok.ptr && tok.length == seg->str_length &&
           strncmp(tok.ptr, seg->str, tok.length) == 0;
}

static segment_t *find_child(segment_t *segment, token_t tok)
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

void graph_stats(const segment_t *seg, graph_stats_t *stats)
{
    stats->nodes++;

    // Literal children.
    stats->symbolic_edges += seg->child_count;
    for (uint16_t i = 0; i < seg->child_count; i++) {
        graph_stats(seg->children[i], stats);
    }

    // Special.
    switch (seg->spec_type) {
        case SPEC_PARAM:
            // Parameters.
            stats->edges++;
            graph_stats(seg->special.param, stats);
            break;

        case SPEC_WILDCARD:
            // Terminal wildcard.
            stats->edges++;
            stats->nodes++;
            stats->terminals++;
            break;

        case SPEC_NONE:
            break;
    }

    // Terminal.
    if (seg->route.handler != NULL)
        stats->terminals++;
}

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

static size_t graph_offset(const void *graph, const void *entry)
{
    return (uint8_t *)entry - (uint8_t *)graph;
}

static node_t *graph_compile(struct router *router, segment_t *segment, size_t *cursor)
{
    void *g = router->graph;

    // Append node.
    node_t *node = graph_append(g, cursor, sizeof(node_t), _Alignof(node_t));

    node->literals = segment->child_count;
    if (segment->route.handler != NULL) {
        node->flags |= NODE_FLAG_TERMINAL;
        router->terminals.refs[router->terminals.count] = graph_offset(g, node);
        router->terminals.base[router->terminals.count++] = segment->route;
    }

    edge_t *p_edge = NULL;
    edge_t *w_edge = NULL;

    switch (segment->spec_type) {
        case SPEC_PARAM:
            node->flags |= NODE_FLAG_HAS_PARAM;
            p_edge = graph_append(g, cursor, sizeof(edge_t), _Alignof(edge_t));
            p_edge->symbol = symbol_resolve(segment->special.param->str, router->params.base,
                                            router->params.count);
            break;

        case SPEC_WILDCARD:
            node->flags |= NODE_FLAG_HAS_WILDCARD;
            w_edge = graph_append(g, cursor, sizeof(edge_t), _Alignof(edge_t));
            break;

        case SPEC_NONE:
            break;
    }

    // Descend into literals.
    edge_t *l_edge_base =
        graph_append(g, cursor, segment->child_count * sizeof(edge_t), _Alignof(edge_t));

    for (uint16_t i = 0; i < segment->child_count; i++) {
        segment_t *child = segment->children[i];
        edge_t *l_edge = &l_edge_base[i];
        l_edge->symbol = symbol_resolve(child->str, router->literals.base, router->literals.count);
    }

    for (uint16_t i = 0; i < segment->child_count; i++) {
        segment_t *child = segment->children[i];

        node_t *l_node = graph_compile(router, child, cursor);

        edge_t *l_edge = &l_edge_base[i];
        l_edge->next = graph_offset(g, l_node);
    }

    // Descend into parameter.
    if (p_edge != NULL) {
        node_t *p_node = graph_compile(router, segment->special.param, cursor);
        p_edge->next = graph_offset(g, p_node);
    }

    // Append wildcard node.
    if (w_edge != NULL) {
        node_t *w_node = graph_append(g, cursor, sizeof(node_t), _Alignof(node_t));
        w_node->flags |= NODE_FLAG_TERMINAL;
        w_edge->next = graph_offset(g, w_node);
        router->terminals.refs[router->terminals.count] = w_edge->next;
        router->terminals.base[router->terminals.count++] = segment->special.wildcard->route;
    }

    return node;
}

static void graph_size(segment_t *segment, size_t *total_size)
{
    size_up(total_size, _Alignof(node_t), sizeof(node_t));

    switch (segment->spec_type) {
        case SPEC_PARAM:
        case SPEC_WILDCARD:
            size_up(total_size, _Alignof(edge_t), sizeof(edge_t));
            break;

        case SPEC_NONE:
            break;
    }

    size_up(total_size, _Alignof(edge_t), segment->child_count * sizeof(edge_t));
    for (uint16_t i = 0; i < segment->child_count; i++) {
        segment_t *child = segment->children[i];
        graph_size(child, total_size);
    }

    switch (segment->spec_type) {
        case SPEC_PARAM:
            graph_size(segment->special.param, total_size);
            break;

        case SPEC_WILDCARD:
            size_up(total_size, _Alignof(node_t), sizeof(node_t));
            break;

        case SPEC_NONE:
            break;
    }
}

symbols_t symbol_compile(const symbol_table_t *tbl)
{
    symbols_t sym = { 0 };

    // Allocate space for the string pointers.
    sym.base = malloc(sizeof(char *) * tbl->count);
    if (sym.base == NULL)
        return sym;

    sym.count = tbl->count;

    // Copy and sort string pointers by string contents.
    memcpy(sym.base, tbl->base, sizeof(char *) * tbl->count);
    qsort(sym.base, sym.count, sizeof(char *), strpcmp);

    // Allocate space for the strings in a contiguous memory region.
    sym.region = malloc(arena_used(&tbl->arena));
    if (sym.region == NULL) {
        free(sym.base);
        return (symbols_t){ 0 };
    }

    // Copy strings from the arena and update the string pointers.
    size_t cursor = 0;
    for (size_t i = 0; i < sym.count; i++) {
        size_t n = strlen(sym.base[i]) + 1;

        // Copy string.
        memcpy(sym.region + cursor, sym.base[i], n);

        // Update pointer to point to the copied string!
        sym.base[i] = sym.region + cursor;

        cursor += n;
    }

    return sym;
}

struct router *wrouter_compile(const struct builder *builder)
{
    wrouter_t *router = calloc(1, sizeof(struct router));
    if (router == NULL)
        return NULL;

    router->lx = calloc(1, sizeof(lexer_t));
    router->literals = symbol_compile(&builder->literals);
    router->params = symbol_compile(&builder->params);
    if (router->lx == NULL || router->literals.base == NULL || router->params.base == NULL) {
        wrouter_free(router);
        return NULL;
    }

    graph_stats_t stats = { 0 };
    graph_stats(builder->root, &stats);

    router->terminals.refs = calloc(stats.terminals, sizeof(uint16_t));
    router->terminals.base = calloc(stats.terminals, sizeof(struct route));

#if 0
#include <stdio.h>
    // Using stats does not work if alignment is broken. Needs to use an actual
    // layout pass calculation.  This is kept here for demonstration.  To break
    // it, add an extra byte to the node struct, which will throw off
    // alignment.
    printf("GRAPH BYTES FIRST PASS: %lu\n", graph_bytes);
    size_t other_bytes = sizeof(node_t) * stats.nodes;
    other_bytes += sizeof(edge_t) * (stats.edges + stats.symbolic_edges);
    printf("GRAPH BYTES STATS: %lu\n", other_bytes);
#endif

    size_t graph_bytes = 0;
    graph_size(builder->root, &graph_bytes);
    void *graph = malloc(graph_bytes);
    if (graph == NULL) {
        wrouter_free(router);
        return NULL;
    }
    router->graph = graph;

    size_t cursor = 0;
    graph_compile(router, builder->root, &cursor);

#if 0
#include <stdio.h>
    // Check that the terminals were saved.
    printf("TERMINAL REFS FOUND: %u\n", router->terminals.count);
    printf("TERMINALS STATS: %lu\n", stats.terminals);
    for (int i = 0; i < stats.terminals; i++) {
        printf("%u ", router->terminals.refs[i]);
    }
    printf("\n");
#endif

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
