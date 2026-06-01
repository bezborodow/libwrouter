#include "wrouter.h"

const char *wrouter_strerror(wrouter_error_t err)
{
    switch (err) {
        case WROUTER_OK:
            return "Success.";

        case WROUTER_ERR_NO_MEMORY:
            return "The operation failed due to insufficient memory.";

        case WROUTER_ERR_ILLEGAL_TOKEN:
            return "The route pattern contains an illegal token.";

        case WROUTER_ERR_DUPLICATE_ROUTE:
            return "The route is a duplicate of an existing route.";

        case WROUTER_ERR_LITERAL_CONFLICTS_WITH_PARAM:
            return "A literal segment conflicts with an existing parameter segment.";

        case WROUTER_ERR_PARAM_CONFLICTS_WITH_WILDCARD:
            return "A parameter segment conflicts with an existing wildcard segment.";

        case WROUTER_ERR_PARAM_CONFLICTS_WITH_LITERAL:
            return "A parameter segment conflicts with an existing literal segment.";

        case WROUTER_ERR_PARAM_NAME_MISMATCH:
            return "The parameter name does not match the existing parameter at this position.";

        case WROUTER_ERR_WILDCARD_CONFLICTS_WITH_PARAM:
            return "A wildcard segment conflicts with an existing parameter segment.";

        case WROUTER_ERR_WILDCARD_NOT_FINAL:
            return "A wildcard segment must be the final segment in the route pattern.";

        default:
            return "An unknown error occurred.";
    }
}
