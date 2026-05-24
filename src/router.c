#include "wrouter.h"
#include "router.h"

void wrouter_dispatch(const wrouter_t *router, const char *path, void *dispatch_ctx)
{
    return;
}

void wrouter_free(wrouter_t *router)
{
    free(router->graph);
    free(router);
}
