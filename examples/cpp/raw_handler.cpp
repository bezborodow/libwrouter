#include "wrouter.hpp"

#include <iostream>
#include <string>

struct Response {
    std::string body;
};

static void handle_raw(void *dispatch_ctx,
                       const void *route_ctx,
                       const wrouter_params_t *params)
{
    auto *response = static_cast<Response *>(dispatch_ctx);
    auto *prefix = static_cast<const char *>(route_ctx);
    const wrouter_param_t *name = wrouter_param(params, "name");

    response->body = std::string(prefix) + ", " +
        std::string(name->value, name->length) + "!";
}

int main()
{
    const char *prefix = "Hello";

    wrouter::Builder builder;
    builder.add_handler("/hello/:name", handle_raw, prefix);

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    Response response;
    dispatcher.dispatch_raw("/hello/world", &response);

    std::cout << response.body << "\n";
}
