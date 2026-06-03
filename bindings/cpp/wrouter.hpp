#pragma once

#include <wrouter.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <stdexcept>

namespace wrouter {

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
    ~Router();

    Router(Router&& rhs) noexcept;
    Router& operator=(Router&& rhs) noexcept;

    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    [[nodiscard]]
    wrouter_t *native() const noexcept;

private:
    wrouter_t *ptr_;
};

class Builder {
public:
    explicit Builder(const wrouter_options_t& opts = {});

    ~Builder();

    Builder(Builder&& rhs) noexcept;
    Builder& operator=(Builder&& rhs) noexcept;

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    Builder& add(std::string_view pattern,
                 wrouter_handler_fn fn,
                 const void *ctx = nullptr);

    Builder& add_context(std::string_view pattern,
                         const void *ctx);

    [[nodiscard]]
    Router consume();

private:
    wrouter_builder_t *ptr_;
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

    void dispatch(std::string_view path,
                  void *dispatch_ctx = nullptr);

    [[nodiscard]]
    std::unordered_map<std::string, std::string>
    params() const;

private:
    wrouter_dispatcher_t *ptr_;
};

template<typename T>
T *Dispatcher::resolve(std::string_view path)
{
    return static_cast<T*>(
        const_cast<void*>(
            wrouter_resolve(ptr_, path.data())
        )
    );
}

}
