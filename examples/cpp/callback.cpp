#include "wrouter.hpp"

#include <iostream>
#include <string>

struct Response {
    std::string body;
};

int main()
{
    wrouter::Builder builder;

    builder.add<Response>("/hello/:name", [](Response& response, wrouter::ParamsView params) {
        response.body = "Hello, " + std::string(params["name"]) + "!";
    });

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    Response response;
    dispatcher.dispatch("/hello/world", response);

    std::cout << response.body << "\n";
}
