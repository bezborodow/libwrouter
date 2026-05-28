#include "wrouter.h"
#include "router.h"
#include "lexer.h"
#include "symbol.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// TODO common graph_offset
static size_t graph_offset(const void *graph, const void *entry)
{
    return (uint8_t *)entry - (uint8_t *)graph;
}

static struct route *terminal_lookup(const struct router *router, uint16_t ref)
{
    // TODO custom binary search.
    const terminals_t *t = &router->terminals;

    for (uint16_t i = 0; i < t->count; i++) {
        if (t->refs[i] == ref) {
            return &t->base[i];
        }
    }
    return NULL;
}

static const struct route *route_match(const struct router *router, struct params *params)
{
    token_t tok = { 0 };
    size_t symbol = 0;

    void *g = router->graph;
    node_t *cur = g, *w_node = NULL;
    edge_t *edge = NULL;

    printf("Symbol count %u\n", router->literals.count);

lexer_next:
    tok = lexer_next(router->lx);
    edge_t *edge_base = (edge_t *)((uint8_t *)cur + sizeof(node_t));

    if (cur->flags & (NODE_FLAG_HAS_PARAM | NODE_FLAG_HAS_WILDCARD))
        edge_base++;

    if (cur->flags & NODE_FLAG_HAS_WILDCARD)
        w_node = cur;

    switch (tok.type) {
        case TOKEN_LITERAL:
            printf("Part %.*s\n", tok.length, tok.ptr);

            if (cur->flags & NODE_FLAG_HAS_WILDCARD)
                w_node = cur;

            symbol = symbol_resolve(tok.ptr, router->literals.base, router->literals.count);
            printf("Resolve %.*s to symbol %lu.\n", tok.length, tok.ptr, symbol);
            if (symbol && cur->literals) {

                // TODO do bsearch if n > 8. Need to sort symbols first though when compiling.
                for (uint16_t i = 0; i < cur->literals; i++) {
                    edge = &edge_base[i];
                    printf("Check symbol %u.\n", edge->symbol);
                    printf("Check symbol of %s\n", router->literals.base[edge->symbol - 1]);
                    if (edge->symbol == symbol) {
                        cur = (node_t *)((uint8_t *)g + edge->next);
                        printf("Follow symbol.\n");
                        goto lexer_next;
                    }
                }
            }

            if (cur->flags & NODE_FLAG_HAS_PARAM) {
                edge = (edge_t *)((uint8_t *)cur + sizeof(node_t));
                cur = (node_t *)((uint8_t *)g + edge->next);

                param_t *param = &params->items[params->count++];
                param->name = router->params.base[edge->symbol - 1];
                param->value = tok.ptr;
                param->length = tok.length;

                printf("Follow param %s.\n", param->name);

                goto lexer_next;
            }

            if (w_node != NULL)
                goto wildcard;

            goto not_found;

        case TOKEN_END:
            printf("End.\n");
            if (cur->flags & NODE_FLAG_TERMINAL) {
                printf("Found terminal.\n");

                return terminal_lookup(router, graph_offset(g, cur));
            }

            goto not_found;
    }

not_found:
    printf("Not found.\n");
    params->count = 0;
    return NULL;

wildcard:
    printf("Found wildcard.\n");
    edge = (edge_t *)((uint8_t *)w_node + sizeof(node_t));
    cur = (node_t *)((uint8_t *)g + edge->next);

    return terminal_lookup(router, graph_offset(g, cur));
}

void wrouter_dispatch(const struct router *router, const char *path, void *dispatch_ctx)
{
    printf("-----------------------\n");
    printf("%s\n", path);
    wrouter_ndispatch(router, path, strlen(path), dispatch_ctx);
}

void wrouter_ndispatch(const struct router *router, const char *path, size_t length,
                       void *dispatch_ctx)
{
    lexer_load(router->lx, path, length);

    struct params params = { 0 };

    const struct route *route = route_match(router, &params);

    if (route == NULL) {
        printf("Calling fallback.\n");
        route = &router->fallback;
    }

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
