#include "wrouter.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>

static void raw_handler(void *dispatch_ctx,
                        const void *route_ctx,
                        const wrouter_params_t *params)
{
    auto *seen = static_cast<bool *>(dispatch_ctx);
    auto *expected = static_cast<const char *>(route_ctx);

    assert(std::string_view{ expected } == "raw");
    assert(params->count == 1);
    assert(std::string_view{ params->base[0].name } == "id");
    assert((std::string_view{ params->base[0].value, params->base[0].length } == "123"));

    *seen = true;
}

static void test_raw_handler()
{
    bool seen = false;
    const char *ctx = "raw";

    wrouter::Builder builder;
    builder.add_handler("/raw/:id", raw_handler, ctx);

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    dispatcher.dispatch("/raw/123", &seen);

    assert(seen);
}

static void test_capturing_handler()
{
    std::string captured = "prefix";
    std::string result;

    wrouter::Builder builder;
    builder.add("/hello/:name", [&](wrouter::ParamsView params) {
        result = captured + ":" + std::string(params["name"]);
    });

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    dispatcher.dispatch("/hello/world");

    assert(result == "prefix:world");
}

static void test_dispatch_context_handler()
{
    wrouter::Builder builder;
    builder.add("/write/:value", [](void *dispatch_ctx, wrouter::ParamsView params) {
        auto *out = static_cast<std::string *>(dispatch_ctx);
        *out = std::string(params["value"]);
    });

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    std::string out;
    dispatcher.dispatch("/write/ok", &out);

    assert(out == "ok");
}

static void test_params_view()
{
    wrouter::Builder builder;
    builder.add_context("/account/:account_id/contact/:contact_id", nullptr);

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    (void)dispatcher.resolve("/account/200/contact/300");

    auto params = dispatcher.params_view();

    assert(params.count() == 2);
    assert(!params.empty());
    assert(params.at(0) == "200");
    assert(params["account_id"] == "200");
    assert(params.get("contact_id") == "300");
    assert(params.get("missing").empty());

    auto copied = dispatcher.params();
    assert(copied["account_id"] == "200");
    assert(copied["contact_id"] == "300");
}

static void test_router_move_keeps_handlers()
{
    int calls = 0;

    wrouter::Builder builder;
    builder.add("/move", [&](wrouter::ParamsView params) {
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

static void ref_noop(const void *)
{}

static void test_callable_rejects_c_reference_callbacks()
{
    wrouter_options_t opts = {};
    opts.retain = ref_noop;

    wrouter::Builder builder(opts);

    try {
        builder.add("/bad", [](wrouter::ParamsView) {});
    } catch (const std::logic_error &) {
        return;
    }

    assert(false);
}

int main()
{
    test_raw_handler();
    test_capturing_handler();
    test_dispatch_context_handler();
    test_params_view();
    test_router_move_keeps_handlers();
    test_callable_rejects_c_reference_callbacks();
}
