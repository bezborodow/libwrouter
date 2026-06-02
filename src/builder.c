#include "token.h"
#include "wrouter.h"
#include "router.h"
#include "builder.h"
#include "prelexer.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

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
    if (builder->retain)
        builder->retain(builder->fallback.ctx);

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
 * Check if a token matches against a given segment.
 */
static bool token_matches(token_t tok, const segment_t *seg)
{
    return seg->str && tok.ptr && tok.length == seg->str_length &&
           strncmp(tok.ptr, seg->str, tok.length) == 0;
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

        if (token_matches(tok, child))
            return child;
    }

    return NULL;
}

/**
 * Add a route handler to the route tree.
 */
int wrouter_add_handler(wrouter_builder_t *builder, const char *pattern, wrouter_handler_fn handler)
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
int wrouter_add_handler_ctx(wrouter_builder_t *builder, const char *pattern,
                            wrouter_handler_fn handler, const void *ctx)
{
    wrouter_route_t route = {
        .handler = handler,
        .ctx = ctx,
    };

    return wrouter_add_route(builder, pattern, route);
}

int wrouter_add_context(wrouter_builder_t *builder, const char *pattern, const void *ctx)
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
int wrouter_add_route(wrouter_builder_t *builder, const char *pattern, wrouter_route_t route)
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
                // Append node terminal route.
                cur->route = route;
                cur->terminal = true;
                if (builder->retain != NULL)
                    builder->retain(route.ctx);

                return 0;

            case TOKEN_LITERAL: {
                // Literals are incompatible with parameters.
                if (cur->spec_type == SPEC_PARAM)
                    return WROUTER_ERR_LITERAL_CONFLICTS_WITH_PARAM;

                // Check for an existing child.
                segment_t *child = find_child_by_token(cur, tok);

                // If not, create one.
                if (child == NULL) {

                    if (cur->child_count >= UINT8_MAX)
                          return WROUTER_ERR_OUT_OF_RANGE;

                    const char *strptr = symbol_append(&builder->literals, tok.ptr, tok.length);
                    if (strptr == NULL)
                        return WROUTER_ERR_NO_MEMORY;

                    // Append child.
                    segment_t **new_children =
                        realloc(cur->children, sizeof(segment_t *) * (cur->child_count + 1));
                    if (new_children == NULL)
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

                if (cur->spec_type == SPEC_PARAM) {

                    // If a parameter is already assigned, it should have the same name.
                    if (!token_matches(tok, cur->special.param))
                        return WROUTER_ERR_PARAM_NAME_MISMATCH;

                } else {

                    // Append parameter.
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

                if (builder->retain != NULL)
                    builder->retain(route.ctx);

                return 0;
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

                if (builder->retain != NULL)
                    builder->retain(route.ctx);

                return 0;
            }

            case TOKEN_ILLEGAL:
            default:
                return WROUTER_ERR_ILLEGAL_PATTERN;
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

static node_t *graph_compile(wrouter_t *router, const segment_t *segment, size_t *cursor)
{
    void *g = router->graph;
    node_t *node = NULL;
    edge_t *p_edge = NULL, *w_edge = NULL, *t_edge = NULL;

    // Append node.
    node = graph_append_node(g, cursor);

    // Store number of literals.
    node->literals = segment->child_count;

    // Terminate node.
    if (segment->terminal) {

        // Store termination flag.
        node->flags |= NODE_FLAG_TERMINAL;

        // Store route in the terminal parallel arrays.
        // refs is the terminating node's graph offset. Searching for the
        // offset yields an index that is used to lookup the terminal in the
        // base array.
        router->terminals.refs[router->terminals.count] = graph_offset(g, node);
        router->terminals.base[router->terminals.count++] = segment->route;

        // Retain context.
        // Callback to retain context reference count for garbage collection if
        // required (for example, the Python library needs this.)
        if (router->retain != NULL)
            router->retain(segment->route.ctx);
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
        router->terminals.base[router->terminals.count++] = segment->special.wildcard->route;

        // Retain context.
        if (router->retain != NULL)
            router->retain(segment->special.wildcard->route.ctx);
    }

    // Append trailing node.
    if (t_edge != NULL) {
        node_t *t_node = graph_append_node(g, cursor);
        t_node->flags |= NODE_FLAG_TERMINAL;
        t_edge->next = graph_offset(g, t_node);
        router->terminals.refs[router->terminals.count] = t_edge->next;
        router->terminals.base[router->terminals.count++] = segment->trailing->route;

        // Retain context.
        if (router->retain != NULL)
            router->retain(segment->trailing->route.ctx);
    }

    return node;
}

/**
 * Calculate graph statistics by traversing the route tree.
 *
 * Amongst other things, this will allow the builder to allocate the exact
 * amount of memory required for building the route graph.
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
    if (seg->terminal)
        stats->terminals++;
}

/**
 * Compiles an alphabetically sorted symbol list from a symbol table.
 *
 * References its own memory region of character strings.
 */
symbols_t symbol_compile(const symbol_table_t *tbl)
{
    symbols_t sym = { 0 };

    sym.count = tbl->count;

    if (!tbl->count || tbl->base == NULL)
        goto failure;

    // Allocate space for the string pointers.
    sym.base = malloc(sizeof(char *) * tbl->count);
    if (sym.base == NULL)
        goto failure;

    // Copy and sort string pointers by string contents.
    memcpy(sym.base, tbl->base, sizeof(char *) * tbl->count);
    qsort(sym.base, sym.count, sizeof(char *), strpcmp);

    // Allocate space for the strings in a contiguous memory region.
    sym.region = malloc(arena_used(&tbl->arena));
    if (sym.region == NULL) {
        goto failure;
    }

    // Copy strings from the arena and update the string pointers.
    for (size_t cursor = 0, i = 0; i < sym.count; i++) {
        size_t n = strlen(sym.base[i]) + 1;

        // Copy string.
        memcpy(sym.region + cursor, sym.base[i], n);

        // Update pointer to point to the copied string!
        sym.base[i] = sym.region + cursor;

        cursor += n;
    }

    return sym;

failure:
    // Return empty symbol list on memory failure.
    free(sym.base);
    return (symbols_t){ 0 };
}

/**
 * Compile the route tree.
 *
 * This will compile an immutable router from a route tree, which is therefore
 * thread-safe. The router consists of a graph, symbols, and terminals.
 */
wrouter_t *wrouter_compile(const wrouter_builder_t *builder)
{
    graph_stats_t stats = { 0 };
    size_t cursor = 0;

    // New router.
    wrouter_t *router = calloc(1, sizeof(wrouter_t));
    if (router == NULL)
        return NULL;

    // Copy options from the builder onto the router.
    router->fallback = builder->fallback;
    router->retain = builder->retain;
    router->release = builder->release;
    if (router->retain != NULL)
        router->retain(router->fallback.ctx);

    // Obtain graph statistics.
    graph_stats(builder->root, &stats);
    router->num_routes = stats.terminals;
    router->max_params = stats.max_params;

    // If no terminals, return an empty router.
    if (!stats.terminals)
        return router;

    // Allocate and compile symbols for literals.
    router->literals = symbol_compile(&builder->literals);
    if (router->literals.base == NULL && router->literals.count)
        goto failure;

    // Allocate and compile symbols for parameters.
    router->params = symbol_compile(&builder->params);
    if (router->params.base == NULL && router->params.count)
        goto failure;

    // Allocate terminal refs.
    router->terminals.refs = calloc(stats.terminals, sizeof(uint16_t));
    if (router->terminals.refs == NULL)
        goto failure;

    // Allocate terminals.
    router->terminals.base = calloc(stats.terminals, sizeof(wrouter_route_t));
    if (router->terminals.base == NULL)
        goto failure;

    // Allocate the graph.
    router->graph = malloc(stats.size);
    if (router->graph == NULL)
        goto failure;

    // Compile the graph.
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
