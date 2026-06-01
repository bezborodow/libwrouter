#include "wrouter.h"
#include "router.h"
#include "lexer.h"
#include "symbol.h"
#include "dispatcher.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

static inline const edge_t *node_edge_base(const node_t *node)
{
    uintptr_t align = _Alignof(edge_t);
    uintptr_t cursor = (uintptr_t)node + sizeof(node_t);
    uintptr_t base = (cursor + align - 1) & ~(align - 1);

    return (const edge_t *)base;
}

size_t graph_offset(const void *graph, const void *entry)
{
    return (const uint8_t *)entry - (const uint8_t *)graph;
}

static inline const node_t *next_node(const uint8_t *graph, const edge_t *edge)
{
    return (const node_t *)(graph + edge->next);
}

static struct route *terminal_lookup(const struct router *router, uint16_t ref)
{
    // TODO custom binary search.
    const terminals_t *t = &router->terminals;

    for (uint16_t i = 0; i < t->count; i++)
        if (t->refs[i] == ref)
            return &t->base[i];

    return NULL;
}

/**
 * Match a route in the dispatcher.
 *
 * @param dispatcher Request dispatcher.
 */
const struct route *route_match(struct dispatcher *d)
{
    token_t tok = { 0 };
    size_t symbol = 0;

    const struct router *router = d->router;
    const void *g = router->graph;
    const node_t *cur = g;
    const edge_t *l_edge = NULL, *s_edge = NULL, *w_edge = NULL, *l_edge_base = NULL;

    // Check for an empty graph, which is valid, but will never match anything.
    if (g == NULL)
        goto not_found;

    // Reset parameter count.
    d->params.count = 0;

lexer_next:

    // Consume next token from the lexer.
    tok = lexer_next(&d->lx);

    // Align the edge base memory location to after the current node.
    l_edge_base = s_edge = node_edge_base(cur);

    // If the node has a special edge, then advance the base edge beyond it.
    // The literal edges start after the special edge, if present.  A special
    // edge is either a parameter or a wildcard. They cannot coexist; that is,
    // there is only ever one or zero special edges.
    if (cur->flags & (NODE_FLAG_HAS_PARAM | NODE_FLAG_HAS_WILDCARD))
        l_edge_base++;

    // Remember the most specific wildcard edge, if present.
    if (cur->flags & NODE_FLAG_HAS_WILDCARD)
        w_edge = s_edge;

    // Process the segment token against the current node.
    switch (tok.type) {

        // Literal string.
        case TOKEN_LITERAL:

            // If this node has literals, try to resolve and match.
            if (cur->literals) {

                // Resolve the literal string to a symbol.
                symbol = symbol_resolve(tok.ptr, router->literals.base, router->literals.count);

                // If the symbol is resolved, try to match against an edge.
                if (symbol) {

                    // TODO do bsearch if n > 8. Need to sort symbols first though when compiling.
                    for (uint16_t i = 0; i < cur->literals; i++) {
                        l_edge = &l_edge_base[i];

                        if (l_edge->symbol == symbol) {
                            cur = next_node(g, l_edge);

                            // Follow symbol.
                            goto lexer_next;
                        }
                    }
                }
            }

            // Check for parameter.
            if (cur->flags & NODE_FLAG_HAS_PARAM) {
                cur = next_node(g, s_edge);

                // Record parameter name and value.
                wrouter_param_t *param = &d->params.base[d->params.count++];
                param->name = router->params.base[s_edge->symbol - 1];
                param->value = tok.ptr;
                param->length = tok.length;

                // Follow parameter.
                goto lexer_next;
            }

            // Terminate at wildcard.
            if (w_edge != NULL)
                goto wildcard;

            // Not found.
            goto not_found;

        // End token.
        case TOKEN_END:

            // Check for terminal.
            if (cur->flags & NODE_FLAG_TERMINAL)
                goto terminal;

            // Not found.
            goto not_found;
    }

not_found:
    d->params.count = 0;
    return NULL;

wildcard:
    // Follow the wildcard edge and terminate.
    cur = next_node(g, w_edge);

terminal:
    return terminal_lookup(router, graph_offset(g, cur));
}

/**
 * Free the router.
 */
void wrouter_free(struct router *router)
{
    if (router == NULL)
        return;

    if (router->release != NULL)
        for (uint16_t i = 0; i < router->terminals.count; i++)
            router->release(router->terminals.base[i].ctx);

    free(router->literals.region);
    free(router->literals.base);
    free(router->params.region);
    free(router->params.base);
    free(router->graph);
    free(router->terminals.base);
    free(router->terminals.refs);
    free(router);
}

/**
 * Number of terminal routes in the router.
 */
size_t wrouter_route_count(const wrouter_t *router)
{
    return router->num_routes;
}
