#include "wrouter.h"
#include "dispatcher.h"
#include "router.h"
#include "lexer.h"
#include "symbol.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

void wrouter_dispatch(struct dispatcher *dispatcher, const char *path, void *dispatch_ctx)
{
    wrouter_ndispatch(dispatcher, path, strlen(path), dispatch_ctx);
}

void wrouter_ndispatch(struct dispatcher *dispatcher, const char *path, size_t length,
                       void *dispatch_ctx)
{
    lexer_load(&dispatcher->lx, path, length);

    const struct route *route = route_match(dispatcher);

    if (route == NULL)
        route = &dispatcher->router->fallback;

    route->handler(dispatch_ctx, route->ctx, &dispatcher->params);
}

wrouter_dispatcher_t *wrouter_dispatcher_create(const wrouter_t *router)
{
    struct dispatcher *dispatcher = calloc(1, sizeof(struct dispatcher));
    if (dispatcher == NULL)
        return NULL;

    if (router->max_params) {
        dispatcher->params.base = calloc(router->max_params, sizeof(wrouter_param_t));
        if (dispatcher->params.base == NULL)
            goto failure;
    }

    dispatcher->router = router;

    return dispatcher;

failure:
    free(dispatcher);
    return NULL;
}

void wrouter_dispatcher_free(wrouter_dispatcher_t *dispatcher)
{
    if (dispatcher == NULL)
        return;

    free(dispatcher->params.base);
    free(dispatcher);
}
