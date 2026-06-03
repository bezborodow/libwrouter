#include "dispatcher.h"
#include "graph.h"
#include "lexer.h"
#include "params.h"
#include "router.h"
#include "symbol.h"
#include "terminal.h"
#include "token.h"
#include "wrouter.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

inline void router_retain(const wrouter_t *router, const wrouter_route_t *route)
{
    if (router->retain != NULL)
        router->retain(route->ctx);
}

/**
 * Match a route in the dispatcher.
 *
 * @param dispatcher Request dispatcher.
 */
const wrouter_route_t *route_match(wrouter_dispatcher_t *d)
{
    token_t tok = { 0 };
    size_t symbol = 0;
    const char *w_param = NULL;

    const wrouter_t *router = d->router;
    const void *g = router->graph;
    const node_t *cur = g;
    const edge_t *l_edge_base = NULL, *l_edge = NULL, *s_edge = NULL, *w_edge = NULL,
                 *t_edge = NULL;

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

    // The trailing-slash edge is stored after the special edge if it exists,
    // otherwise immediately after the node.
    if (cur->flags & NODE_FLAG_HAS_TRAILING)
        t_edge = l_edge_base++;

    // Remember the most specific wildcard edge, if present.
    if (cur->flags & NODE_FLAG_HAS_WILDCARD) {
        w_edge = s_edge;
        w_param = tok.ptr;
    }

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

        // Trailing-slash token.
        case TOKEN_TRAILING:
            if (cur->flags & NODE_FLAG_HAS_TRAILING)
                goto trailing;

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
    // Not found; no parameters.
    d->params.count = 0;
    return NULL;

trailing:
    // Follow the trailing-slash edge, and terminate.
    cur = next_node(g, t_edge);
    goto terminal;

wildcard:
    // Save the wildcard parameter.
    {
        wrouter_param_t *param = &d->params.base[d->params.count++];
        param->name = WILDCARD_PARAM;
        param->value = w_param;
        param->length = d->lx.str + d->lx.length - w_param;
    }

    // Follow the wildcard edge and terminate.
    cur = next_node(g, w_edge);

terminal:
    return terminal_lookup(&router->terminals, graph_offset(g, cur));
}

/**
 * Number of terminal routes in the router.
 */
size_t wrouter_route_count(const wrouter_t *router)
{
    return router->num_routes;
}

/**
 * Free the router.
 */
void wrouter_free(wrouter_t *router)
{
    if (router == NULL)
        return;

    // Release all route contexts.
    if (router->release != NULL) {
        router->release(router->fallback.ctx);

        for (uint16_t i = 0; i < router->terminals.count; i++)
            router->release(router->terminals.base[i].ctx);
    }

    // Free the router.
    symbols_free(&router->literals);
    symbols_free(&router->params);
    terminals_free(&router->terminals);
    free(router->graph);
    free(router);
}
