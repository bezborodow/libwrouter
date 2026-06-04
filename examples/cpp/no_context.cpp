#include "wrouter.hpp"

#include <iostream>
#include <string>

int main()
{
    std::string output;

    wrouter::Builder builder;

    builder.add("/status/:state", [&](wrouter::ParamsView params) {
        output = "status=" + std::string(params["state"]);
    });

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    dispatcher.dispatch("/status/ready");

    std::cout << output << "\n";
}
