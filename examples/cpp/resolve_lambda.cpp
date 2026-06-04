#include "wrouter.hpp"

#include <functional>
#include <iostream>
#include <string>

struct Request {
    std::string user;
};

struct Response {
    std::string body;
};

int main()
{
    using Endpoint = std::function<void(const Request&, Response&, std::string)>;

    Endpoint account_view =
        [](const Request& request, Response& response, std::string account_id) {
            response.body =
                request.user + " requested account " + account_id;
        };

    wrouter::Builder<> builder;
    builder.add_context("/account/a/:account_id", &account_view);

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    auto *endpoint =
        dispatcher.resolve<Endpoint>("/account/a/1234");
    auto params = dispatcher.params();

    Request request{ "alice" };
    Response response;

    (*endpoint)(request, response, params["account_id"]);

    std::cout << response.body << "\n";
}
