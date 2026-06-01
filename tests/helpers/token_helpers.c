#include "token.h"
#include "helpers/token_helpers.h"


const char *token_ident(int type)
{
    switch (type) {
        case TOKEN_ILLEGAL:
            return "TOKEN_ILLEGAL";
        case TOKEN_LITERAL:
            return "TOKEN_LITERAL";
        case TOKEN_PARAM:
            return "TOKEN_PARAM";
        case TOKEN_WILDCARD:
            return "TOKEN_WILDCARD";
        case TOKEN_TRAILING:
            return "TOKEN_TRAILING";
        case TOKEN_END:
            return "TOKEN_END";
        default:
            return "TOKEN_UNKNOWN";
    }
}
