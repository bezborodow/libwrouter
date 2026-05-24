#include "wrouter.h"
#include "router.h"
#include "lexer.h"
#include <stddef.h>
#include <string.h>

void wrouter_dispatch(const wrouter_t *router, const char *path, void *dispatch_ctx)
{
    wrouter_ndispatch(router, path, strlen(path), dispatch_ctx);
}

void wrouter_ndispatch(const wrouter_t *router, const char *path, size_t length, void *dispatch_ctx)
{
    lexer_load(router->lx, path, length);

    token_t tok = { 0 };

    for (;;) {
        tok = lexer_next(router->lx);

        if (tok.type != TOKEN_LITERAL) {
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
