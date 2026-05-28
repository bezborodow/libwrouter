#include "wrouter.h"
#include "router.h"
#include "lexer.h"
#include "symbol.h"
#include <stddef.h>
#include <string.h>

// TODO common graph_offset
static size_t graph_offset(const void *graph, const void *entry)
{
    return (uint8_t *)entry - (uint8_t *)graph;
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

static const struct route *route_match(const struct router *router, struct params *params)
{
    token_t tok = { 0 };
    size_t symbol = 0;

    void *g = router->graph;
    node_t *cur = g;
    edge_t *edge = NULL, *s_edge = NULL, *w_edge = NULL, *edge_base = NULL;

lexer_next:
    tok = lexer_next(router->lx);
    edge_base = s_edge = (edge_t *)((uint8_t *)cur + sizeof(node_t));

    // If the node has a special edge, then advance the base edge beyond it.
    // The literal edges start after the special edge, if present.  A special
    // edge is either a parameter or a wildcard. They cannot coexist; that is,
    // there is only ever one or zero special edges.
    if (cur->flags & (NODE_FLAG_HAS_PARAM | NODE_FLAG_HAS_WILDCARD))
        edge_base++;

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
                        edge = &edge_base[i];

                        if (edge->symbol == symbol) {
                            cur = (node_t *)((uint8_t *)g + edge->next);

                            // Follow symbol.
                            goto lexer_next;
                        }
                    }
                }
            }

            // Check for parameter.
            if (cur->flags & NODE_FLAG_HAS_PARAM) {
                cur = (node_t *)((uint8_t *)g + s_edge->next);

                // Record parameter name and value.
                param_t *param = &params->items[params->count++];
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
    params->count = 0;
    return NULL;

wildcard:
    // Follow the wildcard edge and terminate.
    cur = (node_t *)((uint8_t *)g + w_edge->next);

terminal:
    return terminal_lookup(router, graph_offset(g, cur));
}

void wrouter_dispatch(const struct router *router, const char *path, void *dispatch_ctx)
{
    wrouter_ndispatch(router, path, strlen(path), dispatch_ctx);
}

void wrouter_ndispatch(const struct router *router, const char *path, size_t length,
                       void *dispatch_ctx)
{
    lexer_load(router->lx, path, length);

    struct params params = { 0 };

    const struct route *route = route_match(router, &params);

    if (route == NULL)
        route = &router->fallback;

    route->handler(dispatch_ctx, route->ctx, &params);
}

void wrouter_free(struct router *router)
{
    if (router == NULL)
        return;

    free(router->literals.region);
    free(router->literals.base);
    free(router->params.region);
    free(router->params.base);
    free(router->graph);
    free(router->terminals.base);
    free(router->terminals.refs);
    free(router->lx);
    free(router);
}
