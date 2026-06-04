#include "wrouter.hpp"

#include <iostream>
#include <string>

struct Response {
    std::string body;
};

int main()
{
    std::string greeting = "Hello";

    wrouter::Builder builder;

    builder.add("/hello/:name", [&](void *dispatch_ctx, wrouter::ParamsView params) {
        auto *response = static_cast<Response *>(dispatch_ctx);
        response->body = greeting + ", " + std::string(params["name"]) + "!";
    });

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    Response response;
    dispatcher.dispatch("/hello/world", &response);

    std::cout << response.body << "\n";
}
