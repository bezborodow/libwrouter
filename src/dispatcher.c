#include "wrouter.h"
#include "dispatcher.h"
#include "router.h"
#include "lexer.h"
#include "symbol.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/**
 * Dispatch.
 *
 * @param dispatcher Request dispatcher.
 * @param path Null-terminated string containing the request path.
 * @param dispatch_ctx Request dispatcher context.
 */
void wrouter_dispatch(struct dispatcher *dispatcher, const char *path, void *dispatch_ctx)
{
    wrouter_ndispatch(dispatcher, path, strlen(path), dispatch_ctx);
}

/**
 * Dispatch, n bytes.
 *
 * @param dispatcher Request dispatcher.
 * @param path Request path.
 * @param length Length of the request path.
 * @param dispatch_ctx Request dispatcher context.
 */
void wrouter_ndispatch(struct dispatcher *dispatcher, const char *path, size_t length,
                       void *dispatch_ctx)
{
    lexer_load(&dispatcher->lx, path, length);

    const struct route *route = route_match(dispatcher);

    if (route == NULL)
        route = &dispatcher->router->fallback;

    if (route->handler != NULL)
        route->handler(dispatch_ctx, route->ctx, &dispatcher->params);
}

/**
 * Resolve to context.
 *
 * @param dispatcher Request dispatcher.
 * @param path Null-terminated string containing the request path.
 */
const void *wrouter_resolve(struct dispatcher *dispatcher, const char *path)
{
    return wrouter_nresolve(dispatcher, path, strlen(path));
}

/**
 * Resolve to context, n bytes.
 *
 * @param dispatcher Request dispatcher.
 * @param path Request path.
 * @param length Length of the request path.
 */
const void *wrouter_nresolve(struct dispatcher *dispatcher, const char *path, size_t length)
{
    lexer_load(&dispatcher->lx, path, length);

    const struct route *route = route_match(dispatcher);

    if (route == NULL)
        return dispatcher->router->fallback.ctx;

    return route->ctx;
}

const wrouter_params_t *wrouter_params(const struct dispatcher *dispatcher)
{
    return &dispatcher->params;
}

/**
 * Create a dispatcher. The dispatcher is not thread-safe.
 * 
 * The mutable dispatcher holds parameters and the lexer, which are mutable.  A
 * pointer is given for the immutable router, which is not owned by the
 * dispatcher and can therefore be shared between threads.
 */
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

/**
 * Free the dispatcher.
 *
 * This does not free the router because it may be used by another thread.
 * Instead, free the router separately after freeing all dispatchers or any
 * other resources that may be using the router.
 */
void wrouter_dispatcher_free(wrouter_dispatcher_t *dispatcher)
{
    if (dispatcher == NULL)
        return;

    free(dispatcher->params.base);
    free(dispatcher);
}
