#include "wrouter.h"
#include "router.h"

void wrouter_dispatch(const wrouter_t *router, const char *path, void *dispatch_ctx)
{

}

void wrouter_free(wrouter_t *router)
{
    free(router);
}
