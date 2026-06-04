#include "wrouter.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>

static void test_capturing_handler()
{
    std::string captured = "prefix";
    std::string result;

    wrouter::Builder builder;
    builder.add("/hello/:name", [&](wrouter::Params params) {
        result = captured + ":" + params["name"];
    });

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    dispatcher.dispatch("/hello/world");

    assert(result == "prefix:world");
}

static void test_no_context_handler()
{
    wrouter::Builder builder;
    builder.add("/write/:value", [](wrouter::Params params) {
        assert(params["value"] == "ok");
    });

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    dispatcher.dispatch("/write/ok");
}

struct TypedResponse {
    std::string body;
};

static void test_typed_dispatch_context_handler()
{
    wrouter::Builder<TypedResponse> builder;
    builder.add("/write/:value", [](TypedResponse& response,
                                    wrouter::Params params) {
        response.body = params["value"];
    });

    auto router = builder.consume();
    wrouter::Dispatcher<TypedResponse> dispatcher(router);

    TypedResponse response;
    dispatcher.dispatch("/write/ok", response);

    assert(response.body == "ok");
}

static void test_typed_dispatch_context_consistent_across_routes()
{
    wrouter::Builder<std::string> builder;
    std::string out;

    builder.add("/typed/:value", [](std::string& dest,
                                    wrouter::Params params) {
        dest = params["value"];
    });

    builder.add("/again/:value", [](std::string& dest,
                                    wrouter::Params params) {
        dest += ":" + params["value"];
    });

    auto router = builder.consume();
    wrouter::Dispatcher<std::string> dispatcher(router);

    dispatcher.dispatch("/typed/value", out);
    dispatcher.dispatch("/again/next", out);

    assert(out == "value:next");
}

static void test_resolve_params()
{
    const char *endpoint = "contact.view";

    wrouter::Builder builder;
    builder.add_context("/account/:account_id/contact/:contact_id", endpoint);

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    auto *resolved =
        dispatcher.resolve<const char>("/account/200/contact/300");

    assert(std::string_view{ resolved } == endpoint);

    auto params = dispatcher.params();

    assert(params.count() == 2);
    assert(!params.empty());
    assert(params.at(0) == "200");
    assert(params["account_id"] == "200");
    assert(params.get("contact_id") == "300");
    assert(params.get("missing").empty());
}

static void test_router_move_keeps_handlers()
{
    int calls = 0;

    wrouter::Builder builder;
    builder.add("/move", [&](wrouter::Params params) {
        assert(params.empty());
        calls++;
    });

    auto router = builder.consume();
    auto moved = std::move(router);

    wrouter::Dispatcher dispatcher(moved);
    dispatcher.dispatch("/move");
    dispatcher.dispatch("/move");

    assert(calls == 2);
}

static void test_callable_params_survive_nested_dispatch()
{
    std::string result;
    wrouter::Dispatcher<> *dispatcher_ptr = nullptr;

    wrouter::Builder builder;
    builder.add("/outer/:value", [&](wrouter::Params params) {
        assert(params["value"] == "one");

        dispatcher_ptr->dispatch("/inner/two");

        assert(params["value"] == "one");
        result = params["value"];
    });

    builder.add("/inner/:value", [](wrouter::Params params) {
        assert(params["value"] == "two");
    });

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);
    dispatcher_ptr = &dispatcher;

    dispatcher.dispatch("/outer/one");

    assert(result == "one");
}

static void ref_noop(const void *)
{}

static void test_callable_rejects_c_reference_callbacks()
{
    wrouter_options_t opts = {};
    opts.retain = ref_noop;

    wrouter::Builder builder(opts);

    try {
        builder.add("/bad", [](wrouter::Params) {});
    } catch (const std::logic_error &) {
        return;
    }

    assert(false);
}

int main()
{
    test_capturing_handler();
    test_no_context_handler();
    test_typed_dispatch_context_handler();
    test_typed_dispatch_context_consistent_across_routes();
    test_resolve_params();
    test_router_move_keeps_handlers();
    test_callable_params_survive_nested_dispatch();
    test_callable_rejects_c_reference_callbacks();
}
