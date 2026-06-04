#include "wrouter.hpp"

#include <utility>

namespace wrouter {

Error::Error(wrouter_error_t err)
    : std::runtime_error(wrouter_strerror(err))
    , code_(err)
{}

wrouter_error_t Error::code() const noexcept { return code_; }

Router::Router(wrouter_t *ptr) noexcept
    : ptr_(ptr)
{}

Router::~Router()
{
    if (ptr_)
        wrouter_destroy(&ptr_);
}

Router::Router(Router&& rhs) noexcept
    : ptr_(std::exchange(rhs.ptr_, nullptr))
{}

Router& Router::operator=(Router&& rhs) noexcept
{
    if (this != &rhs) {
        if (ptr_)
            wrouter_destroy(&ptr_);

        ptr_ = std::exchange(rhs.ptr_, nullptr);
    }

    return *this;
}

wrouter_t *Router::native() const noexcept { return ptr_; }

Builder::Builder(const wrouter_options_t& opts)
{
    ptr_ = wrouter_builder_create(opts);

    if (!ptr_)
        throw std::bad_alloc{};
}

Builder::~Builder()
{
    if (ptr_)
        wrouter_builder_destroy(&ptr_);
}

Builder::Builder(Builder&& rhs) noexcept
    : ptr_(std::exchange(rhs.ptr_, nullptr))
{}

Builder& Builder::operator=(Builder&& rhs) noexcept
{
    if (this != &rhs) {
        if (ptr_)
            wrouter_builder_destroy(&ptr_);

        ptr_ = std::exchange(rhs.ptr_, nullptr);
    }

    return *this;
}

Builder& Builder::add(std::string_view pattern,
                      wrouter_handler_fn fn,
                      const void *ctx)
{
    wrouter_error_t err =
        wrouter_add_handler_ctx(ptr_, pattern.data(), fn, ctx);

    if (err)
        throw Error(err);

    return *this;
}

Builder& Builder::add_context(std::string_view pattern,
                              const void *ctx)
{
    auto err = wrouter_add_context(ptr_, pattern.data(), ctx);

    if (err)
        throw Error(err);

    return *this;
}

Router Builder::consume()
{
    wrouter_error_t err = WROUTER_OK;

    wrouter_t *router =
        wrouter_consume(&ptr_, &err);

    if (err)
        throw Error(err);

    return Router(router);
}

Dispatcher::Dispatcher(const Router& router)
{
    ptr_ = wrouter_dispatcher_create(router.native());

    if (!ptr_)
        throw std::bad_alloc{};
}

Dispatcher::~Dispatcher()
{
    if (ptr_)
        wrouter_dispatcher_destroy(&ptr_);
}

void Dispatcher::dispatch(std::string_view path,
                          void *dispatch_ctx)
{
    wrouter_dispatch(ptr_, path.data(), dispatch_ctx);
}

std::unordered_map<std::string, std::string> Dispatcher::params() const
{
    std::unordered_map<std::string, std::string> out;

    auto *p = wrouter_params(ptr_);

    for (uint32_t i = 0; i < p->count; ++i) {
        auto &v = p->base[i];

        out.emplace(
            v.name,
            std::string(v.value, v.length)
        );
    }

    return out;
}

}
