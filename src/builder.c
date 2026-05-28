#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "prelexer.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

static inline uintptr_t align_up(size_t cursor, size_t align)
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

struct builder *wrouter_builder_create(const wrouter_options_t options)
{
    struct builder *builder;

    if (options.fallback_handler == NULL)
        return NULL;

    builder = calloc(1, sizeof(*builder));
    if (builder == NULL)
        return NULL;

    builder->param_syntax = options.param_syntax;
    builder->fallback.handler = options.fallback_handler;
    builder->fallback.ctx = options.fallback_ctx;

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

static int edge_cmp(const void *p1, const void *p2)
{
    const edge_t *e1 = p1;
    const edge_t *e2 = p2;
    return e1->symbol - e2->symbol;
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

static node_t *graph_compile(struct router *router, const segment_t *segment, size_t *cursor)
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
    if (segment->child_count) {
        // Find the start address for literal edges.
        edge_t *l_edge_base =
            graph_append(g, cursor, segment->child_count * sizeof(edge_t), _Alignof(edge_t));

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
        node_t *w_node = graph_append(g, cursor, sizeof(node_t), _Alignof(node_t));
        w_node->flags |= NODE_FLAG_TERMINAL;
        w_edge->next = graph_offset(g, w_node);
        router->terminals.refs[router->terminals.count] = w_edge->next;
        router->terminals.base[router->terminals.count++] = segment->special.wildcard->route;
    }

    return node;
}

void graph_stats(const segment_t *seg, graph_stats_t *stats)
{
    stats->nodes++;
    size_up(&stats->size, _Alignof(node_t), sizeof(node_t));

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

        case SPEC_WILDCARD:
            stats->edges++;
            stats->nodes++;
            stats->terminals++;
            // TODO reserve space for future wildcard param '_' implementation.
            if (stats->param_depth >= stats->max_params)
                stats->max_params++;
            size_up(&stats->size, _Alignof(node_t), sizeof(node_t));
            break;

        case SPEC_NONE:
            break;
    }

    // Terminal node.
    if (seg->route.handler != NULL)
        stats->terminals++;
}

symbols_t symbol_compile(const symbol_table_t *tbl)
{
    symbols_t sym = { 0 };

    sym.count = tbl->count;

    if (!tbl->count || tbl->base == NULL)
        return sym;

    // Allocate space for the string pointers.
    sym.base = malloc(sizeof(char *) * tbl->count);
    if (sym.base == NULL)
        return sym;

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
    graph_stats_t stats = { 0 };
    size_t cursor = 0;
    void *graph;

    wrouter_t *router = calloc(1, sizeof(struct router));
    if (router == NULL)
        return NULL;

    router->fallback = builder->fallback;

    graph_stats(builder->root, &stats);

    // If no terminals, return an empty router.
    if (!stats.terminals)
        return router;

    router->literals = symbol_compile(&builder->literals);
    if (router->literals.base == NULL && router->literals.count)
        goto failure;

    router->params = symbol_compile(&builder->params);
    if (router->params.base == NULL && router->params.count)
        goto failure;

    router->num_routes = stats.terminals;
    router->max_params = stats.max_params;
    router->terminals.refs = calloc(stats.terminals, sizeof(uint16_t));
    if (router->terminals.refs == NULL)
        goto failure;
    router->terminals.base = calloc(stats.terminals, sizeof(struct route));
    if (router->terminals.base == NULL)
        goto failure;

    graph = malloc(stats.size);
    if (graph == NULL)
        goto failure;

    router->graph = graph;

    graph_compile(router, builder->root, &cursor);

    return router;

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
