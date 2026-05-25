#include "wrouter.h"
#include "router.h"
#include "lexer.h"
#include "symbol.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

void wrouter_dispatch(const wrouter_t *router, const char *path, void *dispatch_ctx)
{
    wrouter_ndispatch(router, path, strlen(path), dispatch_ctx);
}

void wrouter_ndispatch(const wrouter_t *router, const char *path, size_t length, void *dispatch_ctx)
{
    lexer_load(router->lx, path, length);

    token_t tok = { 0 };
    size_t symbol = 0;

    node_t *cur = router->graph, *w_node = NULL;

    for (;;) {
        tok = lexer_next(router->lx);

        switch (tok.type) {
            case TOKEN_LITERAL:
                symbol = symbol_resolve(tok.ptr, router->literals.base, router->literals.count);
                if (cur->flags & NODE_FLAG_HAS_PARAM) {
                }
                if (cur->flags & NODE_FLAG_HAS_WILDCARD) {
                    w_node = cur;
                }
                break;

            case TOKEN_END:
                if (cur->flags & NODE_FLAG_TERMINAL) {
                    printf("Found terminal.\n");
                    return;
                }
                if (w_node != NULL) {
                    printf("Found wildcard.\n");
                }
                return;

            case TOKEN_ILLEGAL:
            default:
                return;

        }
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
