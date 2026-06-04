#pragma once

#include <wrouter.h>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace wrouter {

enum class ParamSyntax {
    colon,
    brace,
    angle,
};

template<typename DispatchCtx>
class Builder;

template<typename DispatchCtx>
class Dispatcher;

class Params {
public:
    explicit Params(const wrouter_params_t *params = nullptr);

    [[nodiscard]]
    uint32_t count() const noexcept;

    [[nodiscard]]
    bool empty() const noexcept;

    [[nodiscard]]
    std::string at(uint32_t index) const;

    [[nodiscard]]
    std::string get(std::string_view name) const;

    [[nodiscard]]
    std::string operator[](std::string_view name) const;

    [[nodiscard]]
    const wrouter_params_snapshot_t *native() const noexcept;

private:
    std::shared_ptr<wrouter_params_snapshot_t> snapshot_;
};

namespace detail {

class HandlerBase {
public:
    virtual ~HandlerBase() = default;

    virtual void invoke(void *dispatch_ctx, Params params) const = 0;
};

void handler_trampoline(void *dispatch_ctx,
                        const void *route_ctx,
                        const wrouter_params_t *params);

template<typename DispatchCtx, typename Fn>
class Handler final : public HandlerBase {
public:
    explicit Handler(Fn fn)
        : fn_(std::move(fn))
    {}

    void invoke(void *dispatch_ctx, Params params) const override
    {
        auto *ctx = static_cast<DispatchCtx *>(dispatch_ctx);
        fn_(*ctx, std::move(params));
    }

private:
    mutable Fn fn_;
};

template<typename Fn>
class Handler<void, Fn> final : public HandlerBase {
public:
    explicit Handler(Fn fn)
        : fn_(std::move(fn))
    {}

    void invoke(void *dispatch_ctx, Params params) const override
    {
        (void)dispatch_ctx;
        fn_(std::move(params));
    }

private:
    mutable Fn fn_;
};

class RouterBase {
public:
    explicit RouterBase(wrouter_t *ptr = nullptr) noexcept;
    RouterBase(wrouter_t *ptr, std::vector<std::unique_ptr<HandlerBase>> handlers) noexcept;
    ~RouterBase();

    RouterBase(RouterBase&& rhs) noexcept;
    RouterBase& operator=(RouterBase&& rhs) noexcept;

    RouterBase(const RouterBase&) = delete;
    RouterBase& operator=(const RouterBase&) = delete;

    [[nodiscard]]
    wrouter_t *native() const noexcept;

private:
    wrouter_t *ptr_;
    std::vector<std::unique_ptr<HandlerBase>> handlers_;
};

class BuilderBase {
public:
    explicit BuilderBase(ParamSyntax param_syntax = ParamSyntax::colon);
    ~BuilderBase();

    BuilderBase(BuilderBase&& rhs) noexcept;
    BuilderBase& operator=(BuilderBase&& rhs) noexcept;

    BuilderBase(const BuilderBase&) = delete;
    BuilderBase& operator=(const BuilderBase&) = delete;

    void install_handler(std::string_view pattern,
                         std::unique_ptr<HandlerBase> handler);

    void add_context(std::string_view pattern,
                     const void *ctx);

    [[nodiscard]]
    RouterBase consume();

private:
    wrouter_builder_t *ptr_;
    std::vector<std::unique_ptr<HandlerBase>> handlers_;
};

class DispatcherBase {
public:
    explicit DispatcherBase(const RouterBase& router);
    ~DispatcherBase();

    DispatcherBase(const DispatcherBase&) = delete;
    DispatcherBase& operator=(const DispatcherBase&) = delete;

    [[nodiscard]]
    const void *resolve_raw(std::string_view path);

    void dispatch_impl(std::string_view path,
                       void *dispatch_ctx = nullptr);

    [[nodiscard]]
    Params params() const;

private:
    wrouter_dispatcher_t *ptr_;
};

}

class Error : public std::runtime_error {
public:
    explicit Error(wrouter_error_t err);

    [[nodiscard]]
    wrouter_error_t code() const noexcept;

private:
    wrouter_error_t code_;
};

template<typename DispatchCtx = void>
class Router : private detail::RouterBase {
public:
    explicit Router(wrouter_t *ptr = nullptr) noexcept
        : detail::RouterBase(ptr)
    {}

    Router(Router&& rhs) noexcept = default;
    Router& operator=(Router&& rhs) noexcept = default;

    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    [[nodiscard]]
    wrouter_t *native() const noexcept
    {
        return detail::RouterBase::native();
    }

private:
    friend class Builder<DispatchCtx>;
    friend class Dispatcher<DispatchCtx>;

    explicit Router(detail::RouterBase base) noexcept
        : detail::RouterBase(std::move(base))
    {}
};

template<typename DispatchCtx = void>
class Builder : private detail::BuilderBase {
public:
    explicit Builder(ParamSyntax param_syntax = ParamSyntax::colon)
        : detail::BuilderBase(param_syntax)
    {}

    Builder(Builder&& rhs) noexcept = default;
    Builder& operator=(Builder&& rhs) noexcept = default;

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    template<typename Fn>
    Builder& add(std::string_view pattern, Fn&& fn);

    Builder& add_context(std::string_view pattern,
                         const void *ctx)
    {
        detail::BuilderBase::add_context(pattern, ctx);
        return *this;
    }

    [[nodiscard]]
    Router<DispatchCtx> consume()
    {
        return Router<DispatchCtx>{ detail::BuilderBase::consume() };
    }
};

template<typename DispatchCtx = void>
class Dispatcher : private detail::DispatcherBase {
public:
    explicit Dispatcher(const Router<DispatchCtx>& router)
        : detail::DispatcherBase(router)
    {
        static_assert(
            !std::is_void_v<DispatchCtx>,
            "Dispatcher<void> is handled by the no-context specialization"
        );
    }

    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    template<typename T = void>
    [[nodiscard]]
    T *resolve(std::string_view path);

    void dispatch(std::string_view path, DispatchCtx& dispatch_ctx);

    [[nodiscard]]
    Params params() const
    {
        return detail::DispatcherBase::params();
    }
};

template<>
class Dispatcher<void> : private detail::DispatcherBase {
public:
    explicit Dispatcher(const Router<void>& router)
        : detail::DispatcherBase(router)
    {}

    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    template<typename T = void>
    [[nodiscard]]
    T *resolve(std::string_view path);

    void dispatch(std::string_view path)
    {
        detail::DispatcherBase::dispatch_impl(path);
    }

    [[nodiscard]]
    Params params() const
    {
        return detail::DispatcherBase::params();
    }
};

template<typename DispatchCtx>
Dispatcher(const Router<DispatchCtx>&) -> Dispatcher<DispatchCtx>;

template<typename DispatchCtx>
template<typename Fn>
Builder<DispatchCtx>& Builder<DispatchCtx>::add(std::string_view pattern, Fn&& fn)
{
    static_assert(
        !std::is_pointer_v<DispatchCtx>,
        "dispatch context type must be the pointed-to type, not a pointer type"
    );

    if constexpr (std::is_void_v<DispatchCtx>) {
        static_assert(
            std::is_invocable_v<std::decay_t<Fn>&, Params>,
            "handler must be invocable as fn(Params)"
        );
    } else {
        static_assert(
            std::is_invocable_v<std::decay_t<Fn>&, DispatchCtx&, Params>,
            "handler must be invocable as fn(DispatchCtx&, Params)"
        );
    }

    using Handler = detail::Handler<DispatchCtx, std::decay_t<Fn>>;

    auto handler = std::make_unique<Handler>(std::forward<Fn>(fn));
    detail::BuilderBase::install_handler(pattern, std::move(handler));

    return *this;
}

template<typename DispatchCtx>
template<typename T>
T *Dispatcher<DispatchCtx>::resolve(std::string_view path)
{
    return static_cast<T*>(
        const_cast<void*>(
            detail::DispatcherBase::resolve_raw(path)
        )
    );
}

template<typename DispatchCtx>
void Dispatcher<DispatchCtx>::dispatch(std::string_view path, DispatchCtx& dispatch_ctx)
{
    detail::DispatcherBase::dispatch_impl(path, &dispatch_ctx);
}

template<typename T>
T *Dispatcher<void>::resolve(std::string_view path)
{
    return static_cast<T*>(
        const_cast<void*>(
            detail::DispatcherBase::resolve_raw(path)
        )
    );
}

}
