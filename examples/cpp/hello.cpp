#include "wrouter.hpp"
#include <iostream>

int main()
{
    wrouter::Builder<> builder;

    builder.add_context("/account", "account.list");
    builder.add_context("/account/create", "account.create");
    builder.add_context("/account/a/:account_id", "account.view");

    auto router = builder.consume();

    wrouter::Dispatcher dispatcher(router);

    auto *endpoint =
        dispatcher.resolve<const char>("/account/a/1234");

    auto params = dispatcher.params();

    std::cout
        << endpoint
        << " "
        << params["account_id"]
        << "\n";
}
