#include "wrouter.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>

static void test_errors(void)
{
    assert(strcmp(wrouter_strerror(WROUTER_OK), "Success.") == 0);
    assert(strcmp(wrouter_strerror(WROUTER_ERR_NO_MEMORY), "Insufficient memory.") == 0);
    assert(strcmp(wrouter_strerror(WROUTER_ERR_OUT_OF_RANGE), "Value or index out of range.") == 0);
    assert(strcmp(wrouter_strerror(WROUTER_ERR_ILLEGAL_PATTERN), "Malformed route pattern.") == 0);
    assert(strcmp(wrouter_strerror(WROUTER_ERR_DUPLICATE_ROUTE), "Duplicate route.") == 0);

    assert(strcmp(wrouter_strerror(WROUTER_ERR_LITERAL_CONFLICTS_WITH_PARAM),
                  "A literal segment conflicts with an existing parameter segment.") == 0);

    assert(strcmp(wrouter_strerror(WROUTER_ERR_PARAM_CONFLICTS_WITH_WILDCARD),
                  "A parameter segment conflicts with an existing wildcard segment.") == 0);

    assert(strcmp(wrouter_strerror(WROUTER_ERR_PARAM_CONFLICTS_WITH_LITERAL),
                  "A parameter segment conflicts with an existing literal segment.") == 0);

    assert(strcmp(wrouter_strerror(WROUTER_ERR_PARAM_NAME_MISMATCH),
                  "The parameter name does not match the existing parameter at this position.") == 0);

    assert(strcmp(wrouter_strerror(WROUTER_ERR_WILDCARD_CONFLICTS_WITH_PARAM),
                  "A wildcard segment conflicts with an existing parameter segment.") == 0);

    assert(strcmp(wrouter_strerror(WROUTER_ERR_WILDCARD_NOT_FINAL),
                  "A wildcard segment must be the final segment in the route pattern.") == 0);

    /* unknown/default branch */
    assert(strcmp(wrouter_strerror((wrouter_error_t)9999),
                  "An unknown error occurred.") == 0);
}

int main(void)
{
    test_errors();
    return 0;
}
