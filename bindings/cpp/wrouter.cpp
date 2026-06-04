#include "wrouter.hpp"

namespace wrouter {

ParamsView::ParamsView(const wrouter_params_t *params) noexcept
    : params_(params)
{}

uint32_t ParamsView::count() const noexcept
{
    return params_ ? params_->count : 0;
}

bool ParamsView::empty() const noexcept
{
    return count() == 0;
}

std::string_view ParamsView::at(uint32_t index) const noexcept
{
    if (!params_ || index >= params_->count)
        return {};

    const auto &param = params_->base[index];
    return { param.value, param.length };
}

std::string_view ParamsView::get(std::string_view name) const noexcept
{
    if (!params_)
        return {};

    for (uint32_t i = 0; i < params_->count; ++i) {
        const auto &param = params_->base[i];

        if (name == param.name)
            return { param.value, param.length };
    }

    return {};
}

std::string_view ParamsView::operator[](std::string_view name) const noexcept
{
    return get(name);
}

const wrouter_params_t *ParamsView::native() const noexcept
{
    return params_;
}

namespace detail {

void handler_trampoline(void *dispatch_ctx,
                        const void *route_ctx,
                        const wrouter_params_t *params)
{
    auto *handler = static_cast<const HandlerBase *>(route_ctx);
    handler->invoke(dispatch_ctx, ParamsView{ params });
}

}

Error::Error(wrouter_error_t err)
    : std::runtime_error(wrouter_strerror(err))
    , code_(err)
{}

wrouter_error_t Error::code() const noexcept { return code_; }

Router::Router(wrouter_t *ptr) noexcept
    : ptr_(ptr)
{}

Router::Router(wrouter_t *ptr, std::vector<std::unique_ptr<detail::HandlerBase>> handlers) noexcept
    : ptr_(ptr)
    , handlers_(std::move(handlers))
{}

Router::~Router()
{
    if (ptr_)
        wrouter_destroy(&ptr_);
}

Router::Router(Router&& rhs) noexcept
    : ptr_(std::exchange(rhs.ptr_, nullptr))
    , handlers_(std::move(rhs.handlers_))
{}

Router& Router::operator=(Router&& rhs) noexcept
{
    if (this != &rhs) {
        if (ptr_)
            wrouter_destroy(&ptr_);

        ptr_ = std::exchange(rhs.ptr_, nullptr);
        handlers_ = std::move(rhs.handlers_);
    }

    return *this;
}

wrouter_t *Router::native() const noexcept { return ptr_; }

Builder::Builder(const wrouter_options_t& opts)
    : has_reference_callbacks_(opts.retain != nullptr || opts.release != nullptr)
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
    , has_reference_callbacks_(std::exchange(rhs.has_reference_callbacks_, false))
    , handlers_(std::move(rhs.handlers_))
{}

Builder& Builder::operator=(Builder&& rhs) noexcept
{
    if (this != &rhs) {
        if (ptr_)
            wrouter_builder_destroy(&ptr_);

        ptr_ = std::exchange(rhs.ptr_, nullptr);
        has_reference_callbacks_ = std::exchange(rhs.has_reference_callbacks_, false);
        handlers_ = std::move(rhs.handlers_);
    }

    return *this;
}

Builder& Builder::add_handler(std::string_view pattern,
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

    return Router(router, std::move(handlers_));
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

void Dispatcher::dispatch(std::string_view path)
{
    dispatch_raw(path);
}

void Dispatcher::dispatch_raw(std::string_view path,
                              void *dispatch_ctx)
{
    wrouter_dispatch(ptr_, path.data(), dispatch_ctx);
}

ParamsView Dispatcher::params_view() const noexcept
{
    return ParamsView{ wrouter_params(ptr_) };
}

std::unordered_map<std::string, std::string> Dispatcher::params() const
{
    std::unordered_map<std::string, std::string> out;

    auto *p = params_view().native();

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
