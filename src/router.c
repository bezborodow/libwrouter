#include "wrouter.h"
#include "router.h"
#include "lexer.h"
#include "symbol.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

void wrouter_dispatch(const wrouter_t *router, const char *path, void *dispatch_ctx)
{
    printf("-----------------------\n");
    printf("%s\n", path);
    wrouter_ndispatch(router, path, strlen(path), dispatch_ctx);
}

void wrouter_ndispatch(const wrouter_t *router, const char *path, size_t length, void *dispatch_ctx)
{
    lexer_load(router->lx, path, length);

    token_t tok = { 0 };
    size_t symbol = 0;

    void *g = router->graph;
    node_t *cur = g, *w_node = NULL;
    edge_t *edge = NULL;

    printf("Symbol count %u\n", router->literals.count);

lexer_next:
    tok = lexer_next(router->lx);
    edge_t *edge_base = (edge_t *)((uint8_t *)cur + sizeof(node_t));

    if (cur->flags & (NODE_FLAG_HAS_PARAM | NODE_FLAG_HAS_WILDCARD)) {
        edge_base++;
    }
    if (cur->flags & NODE_FLAG_HAS_WILDCARD) {
        w_node = cur;
    }

    switch (tok.type) {
        case TOKEN_LITERAL:
            printf("Part %.*s\n", tok.length, tok.ptr);
            if (cur->flags & NODE_FLAG_HAS_WILDCARD) {
                w_node = cur;
            }
            if (cur->literals) {
                symbol = symbol_resolve(tok.ptr, router->literals.base, router->literals.count);
                printf("Resolve %.*s to symbol %lu.\n", tok.length, tok.ptr, symbol);

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
                printf("Has param.\n");
                edge = (edge_t *)((uint8_t *)cur + sizeof(node_t));
                cur = (node_t *)((uint8_t *)g + edge->next);
                printf("Follow param.\n");
                goto lexer_next;
            }
            if (w_node != NULL) {
                goto lexer_next;
            }

            return;
            break;

        case TOKEN_END:
            printf("End.\n");
            if (cur->flags & NODE_FLAG_TERMINAL) {
                printf("Found terminal.\n");
                return;
            }
            if (w_node != NULL) {
                printf("Found wildcard.\n");
                edge = (edge_t *)((uint8_t *)w_node + sizeof(node_t));
                cur = (node_t *)((uint8_t *)g + edge->next);
            }
            return;

        case TOKEN_ILLEGAL:
        default:
            return;

    }
}

void wrouter_free(wrouter_t *router)
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
