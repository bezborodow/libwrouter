#include "wrouter.h"

const char *wrouter_strerror(wrouter_error_t err)
{
    switch (err) {
        case WROUTER_OK:
            return "Success.";

        case WROUTER_ERR_NO_MEMORY:
            return "Insufficient memory.";

        case WROUTER_ERR_OUT_OF_RANGE:
            return "Value or index out of range.";

        case WROUTER_ERR_ILLEGAL_PATTERN:
            return "Malformed route pattern.";

        case WROUTER_ERR_DUPLICATE_ROUTE:
            return "Duplicate route.";

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

        case WROUTER_ERR_NULL_ARGUMENT:
            return "Null argument.";

        default:
            return "An unknown error occurred.";
    }
}
