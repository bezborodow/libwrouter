#include "wrouter.h"
#include "router.h"

void wrouter_dispatch(const wrouter_t *router, const char *path, void *dispatch_ctx)
{
    return;
}

void wrouter_free(wrouter_t *router)
{
    free(router->literals.region);
    free(router->literals.base);
    free(router->params.region);
    free(router->params.base);
    free(router->graph);
    free(router);
}
