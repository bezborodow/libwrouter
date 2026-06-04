#include "wrouter.hpp"

namespace wrouter {

Params::Params(const wrouter_params_t *params)
{
    if (!params)
        return;

    snapshot_.reset(wrouter_params_copy(params), wrouter_snapshot_free);

    if (!snapshot_)
        throw std::bad_alloc{};
}

uint32_t Params::count() const noexcept
{
    return snapshot_ ? snapshot_->params.count : 0;
}

bool Params::empty() const noexcept
{
    return count() == 0;
}

std::string Params::at(uint32_t index) const
{
    if (!snapshot_ || index >= snapshot_->params.count)
        return {};

    const auto &param = snapshot_->params.base[index];
    return param.value;
}

std::string Params::get(std::string_view name) const
{
    if (!snapshot_)
        return {};

    for (uint32_t i = 0; i < snapshot_->params.count; ++i) {
        const auto &param = snapshot_->params.base[i];

        if (name == param.name)
            return param.value;
    }

    return {};
}

std::string Params::operator[](std::string_view name) const
{
    return get(name);
}

const wrouter_params_snapshot_t *Params::native() const noexcept
{
    return snapshot_.get();
}

namespace detail {

void handler_trampoline(void *dispatch_ctx,
                        const void *route_ctx,
                        const wrouter_params_t *params)
{
    auto *handler = static_cast<const HandlerBase *>(route_ctx);
    handler->invoke(dispatch_ctx, Params{ params });
}

}

Error::Error(wrouter_error_t err)
    : std::runtime_error(wrouter_strerror(err))
    , code_(err)
{}

wrouter_error_t Error::code() const noexcept { return code_; }

namespace detail {

RouterBase::RouterBase(wrouter_t *ptr) noexcept
    : ptr_(ptr)
{}

RouterBase::RouterBase(wrouter_t *ptr, std::vector<std::unique_ptr<HandlerBase>> handlers) noexcept
    : ptr_(ptr)
    , handlers_(std::move(handlers))
{}

RouterBase::~RouterBase()
{
    if (ptr_)
        wrouter_destroy(&ptr_);
}

RouterBase::RouterBase(RouterBase&& rhs) noexcept
    : ptr_(std::exchange(rhs.ptr_, nullptr))
    , handlers_(std::move(rhs.handlers_))
{}

RouterBase& RouterBase::operator=(RouterBase&& rhs) noexcept
{
    if (this != &rhs) {
        if (ptr_)
            wrouter_destroy(&ptr_);

        ptr_ = std::exchange(rhs.ptr_, nullptr);
        handlers_ = std::move(rhs.handlers_);
    }

    return *this;
}

wrouter_t *RouterBase::native() const noexcept { return ptr_; }

BuilderBase::BuilderBase(const wrouter_options_t& opts)
    : has_reference_callbacks_(opts.retain != nullptr || opts.release != nullptr)
{
    ptr_ = wrouter_builder_create(opts);

    if (!ptr_)
        throw std::bad_alloc{};
}

BuilderBase::~BuilderBase()
{
    if (ptr_)
        wrouter_builder_destroy(&ptr_);
}

BuilderBase::BuilderBase(BuilderBase&& rhs) noexcept
    : ptr_(std::exchange(rhs.ptr_, nullptr))
    , has_reference_callbacks_(std::exchange(rhs.has_reference_callbacks_, false))
    , handlers_(std::move(rhs.handlers_))
{}

BuilderBase& BuilderBase::operator=(BuilderBase&& rhs) noexcept
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

void BuilderBase::install_handler(std::string_view pattern,
                                  std::unique_ptr<HandlerBase> handler)
{
    if (has_reference_callbacks_) {
        throw std::logic_error{
            "C++ callable handlers cannot be used with C retain/release callbacks"
        };
    }

    auto *ctx = handler.get();

    wrouter_error_t err =
        wrouter_add_handler_ctx(ptr_, pattern.data(), handler_trampoline, ctx);

    if (err)
        throw Error(err);

    handlers_.push_back(std::move(handler));
}

void BuilderBase::add_context(std::string_view pattern,
                              const void *ctx)
{
    auto err = wrouter_add_context(ptr_, pattern.data(), ctx);

    if (err)
        throw Error(err);
}

RouterBase BuilderBase::consume()
{
    wrouter_error_t err = WROUTER_OK;

    wrouter_t *router =
        wrouter_consume(&ptr_, &err);

    if (err)
        throw Error(err);

    return RouterBase(router, std::move(handlers_));
}

DispatcherBase::DispatcherBase(const RouterBase& router)
{
    ptr_ = wrouter_dispatcher_create(router.native());

    if (!ptr_)
        throw std::bad_alloc{};
}

DispatcherBase::~DispatcherBase()
{
    if (ptr_)
        wrouter_dispatcher_destroy(&ptr_);
}

const void *DispatcherBase::resolve_raw(std::string_view path)
{
    return wrouter_resolve(ptr_, path.data());
}

void DispatcherBase::dispatch_impl(std::string_view path,
                                   void *dispatch_ctx)
{
    wrouter_dispatch(ptr_, path.data(), dispatch_ctx);
}

Params DispatcherBase::params() const
{
    return Params{ wrouter_params(ptr_) };
}

}

}
