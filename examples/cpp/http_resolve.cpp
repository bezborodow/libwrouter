#include "wrouter.hpp"

#include <functional>
#include <iostream>
#include <string>

struct HttpRequest {
    std::string path;
    std::string user;
};

struct HttpResponse {
    int status = 200;
    std::string body;
};

int main()
{
    using Endpoint =
        std::function<void(const HttpRequest&, HttpResponse&, std::string)>;

    Endpoint account_view =
        [](const HttpRequest& request,
           HttpResponse& response,
           std::string account_id) {
            response.body =
                request.user + " requested account " + account_id;
        };

    wrouter::Builder<> builder;
    builder.add_context("/account/a/:account_id", &account_view);

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    HttpRequest request{ "/account/a/1234", "alice" };
    HttpResponse response;

    auto *endpoint = dispatcher.resolve<Endpoint>(request.path);
    if (endpoint == nullptr) {
        response.status = 404;
        response.body = "not found";
    } else {
        auto params = dispatcher.params();
        (*endpoint)(request, response, params["account_id"]);
    }

    std::cout
        << response.status
        << " "
        << response.body
        << "\n";
}
