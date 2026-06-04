#pragma once

#include <wrouter.h>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace wrouter {

class ParamsView {
public:
    explicit ParamsView(const wrouter_params_t *params = nullptr) noexcept;

    [[nodiscard]]
    uint32_t count() const noexcept;

    [[nodiscard]]
    bool empty() const noexcept;

    [[nodiscard]]
    std::string_view at(uint32_t index) const noexcept;

    [[nodiscard]]
    std::string_view get(std::string_view name) const noexcept;

    [[nodiscard]]
    std::string_view operator[](std::string_view name) const noexcept;

    [[nodiscard]]
    const wrouter_params_t *native() const noexcept;

private:
    const wrouter_params_t *params_;
};

namespace detail {

class HandlerBase {
public:
    virtual ~HandlerBase() = default;

    virtual void invoke(void *dispatch_ctx, ParamsView params) const = 0;
};

void handler_trampoline(void *dispatch_ctx,
                        const void *route_ctx,
                        const wrouter_params_t *params);

template<typename Fn>
class Handler final : public HandlerBase {
public:
    explicit Handler(Fn fn)
        : fn_(std::move(fn))
    {}

    void invoke(void *dispatch_ctx, ParamsView params) const override
    {
        (void)dispatch_ctx;
        fn_(params);
    }

private:
    mutable Fn fn_;
};

template<typename DispatchCtx, typename Fn>
class TypedHandler final : public HandlerBase {
public:
    explicit TypedHandler(Fn fn)
        : fn_(std::move(fn))
    {}

    void invoke(void *dispatch_ctx, ParamsView params) const override
    {
        auto *ctx = static_cast<DispatchCtx *>(dispatch_ctx);
        fn_(*ctx, params);
    }

private:
    mutable Fn fn_;
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

class Router {
public:
    explicit Router(wrouter_t *ptr = nullptr) noexcept;
    Router(wrouter_t *ptr, std::vector<std::unique_ptr<detail::HandlerBase>> handlers) noexcept;
    ~Router();

    Router(Router&& rhs) noexcept;
    Router& operator=(Router&& rhs) noexcept;

    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    [[nodiscard]]
    wrouter_t *native() const noexcept;

private:
    wrouter_t *ptr_;
    std::vector<std::unique_ptr<detail::HandlerBase>> handlers_;
};

class Builder {
public:
    explicit Builder(const wrouter_options_t& opts = {});

    ~Builder();

    Builder(Builder&& rhs) noexcept;
    Builder& operator=(Builder&& rhs) noexcept;

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    Builder& add_handler(std::string_view pattern,
                         wrouter_handler_fn fn,
                         const void *ctx = nullptr);

    template<typename Fn>
    Builder& add(std::string_view pattern, Fn&& fn);

    template<typename DispatchCtx, typename Fn>
    Builder& add(std::string_view pattern, Fn&& fn);

    Builder& add_context(std::string_view pattern,
                         const void *ctx);

    [[nodiscard]]
    Router consume();

private:
    wrouter_builder_t *ptr_;
    bool has_reference_callbacks_;
    std::vector<std::unique_ptr<detail::HandlerBase>> handlers_;
};

class Dispatcher {
public:
    explicit Dispatcher(const Router& router);

    ~Dispatcher();

    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    template<typename T = void>
    [[nodiscard]]
    T *resolve(std::string_view path);

    void dispatch(std::string_view path);

    void dispatch_raw(std::string_view path,
                      void *dispatch_ctx = nullptr);

    template<typename DispatchCtx>
    void dispatch(std::string_view path, DispatchCtx& dispatch_ctx);

    [[nodiscard]]
    ParamsView params_view() const noexcept;

    [[nodiscard]]
    std::unordered_map<std::string, std::string>
    params() const;

private:
    wrouter_dispatcher_t *ptr_;
};

template<typename Fn>
Builder& Builder::add(std::string_view pattern, Fn&& fn)
{
    if (has_reference_callbacks_) {
        throw std::logic_error{
            "C++ callable handlers cannot be used with C retain/release callbacks"
        };
    }

    using Handler = detail::Handler<std::decay_t<Fn>>;

    static_assert(
        std::is_invocable_v<std::decay_t<Fn>&, ParamsView>,
        "handler must be invocable as fn(ParamsView)"
    );

    auto handler = std::make_unique<Handler>(std::forward<Fn>(fn));
    auto *ctx = handler.get();

    wrouter_error_t err =
        wrouter_add_handler_ctx(ptr_, pattern.data(), detail::handler_trampoline, ctx);

    if (err)
        throw Error(err);

    handlers_.push_back(std::move(handler));

    return *this;
}

template<typename DispatchCtx, typename Fn>
Builder& Builder::add(std::string_view pattern, Fn&& fn)
{
    if (has_reference_callbacks_) {
        throw std::logic_error{
            "C++ callable handlers cannot be used with C retain/release callbacks"
        };
    }

    static_assert(
        !std::is_pointer_v<DispatchCtx>,
        "dispatch context type must be the pointed-to type, not a pointer type"
    );

    using Handler = detail::TypedHandler<DispatchCtx, std::decay_t<Fn>>;

    static_assert(
        std::is_invocable_v<std::decay_t<Fn>&, DispatchCtx&, ParamsView>,
        "handler must be invocable as fn(DispatchCtx&, ParamsView)"
    );

    auto handler = std::make_unique<Handler>(std::forward<Fn>(fn));
    auto *ctx = handler.get();

    wrouter_error_t err =
        wrouter_add_handler_ctx(ptr_, pattern.data(), detail::handler_trampoline, ctx);

    if (err)
        throw Error(err);

    handlers_.push_back(std::move(handler));

    return *this;
}

template<typename T>
T *Dispatcher::resolve(std::string_view path)
{
    return static_cast<T*>(
        const_cast<void*>(
            wrouter_resolve(ptr_, path.data())
        )
    );
}

template<typename DispatchCtx>
void Dispatcher::dispatch(std::string_view path, DispatchCtx& dispatch_ctx)
{
    static_assert(
        !std::is_pointer_v<DispatchCtx>,
        "use dispatch_raw() for raw pointer dispatch contexts"
    );

    dispatch_raw(path, &dispatch_ctx);
}

}
