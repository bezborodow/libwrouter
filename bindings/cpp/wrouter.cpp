#include <wrouter.h>

#include <string_view>
#include <unordered_map>
#include <stdexcept>
#include <utility>

namespace wrouter {

class Error : public std::runtime_error {
public:
    explicit Error(wrouter_error_t err)
        : std::runtime_error(wrouter_strerror(err))
        , code_(err)
    {}

    wrouter_error_t code() const noexcept { return code_; }

private:
    wrouter_error_t code_;
};

class Router {
public:
    explicit Router(wrouter_t *ptr = nullptr) noexcept
        : ptr_(ptr)
    {}

    ~Router() {
        if (ptr_)
            wrouter_destroy(&ptr_);
    }

    Router(Router&& rhs) noexcept
        : ptr_(std::exchange(rhs.ptr_, nullptr))
    {}

    Router& operator=(Router&& rhs) noexcept {
        if (this != &rhs) {
            if (ptr_)
                wrouter_destroy(&ptr_);
            ptr_ = std::exchange(rhs.ptr_, nullptr);
        }
        return *this;
    }

    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    wrouter_t *native() const noexcept { return ptr_; }

private:
    wrouter_t *ptr_;
};

class Builder {
public:
    explicit Builder(const wrouter_options_t& opts = {})
    {
        ptr_ = wrouter_builder_create(opts);

        if (!ptr_)
            throw std::bad_alloc{};
    }

    ~Builder() {
        if (ptr_)
            wrouter_builder_destroy(&ptr_);
    }

    Builder(Builder&& rhs) noexcept
        : ptr_(std::exchange(rhs.ptr_, nullptr))
    {}

    Builder& operator=(Builder&& rhs) noexcept {
        if (this != &rhs) {
            if (ptr_)
                wrouter_builder_destroy(&ptr_);

            ptr_ = std::exchange(rhs.ptr_, nullptr);
        }

        return *this;
    }

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    Builder& add(std::string_view pattern,
                 wrouter_handler_fn fn,
                 const void *ctx = nullptr)
    {
        wrouter_error_t err =
            wrouter_add_handler_ctx(ptr_, pattern.data(), fn, ctx);

        if (err)
            throw Error(err);

        return *this;
    }

    Builder& add_context(std::string_view pattern,
                         const void *ctx)
    {
        auto err = wrouter_add_context(ptr_, pattern.data(), ctx);

        if (err)
            throw Error(err);

        return *this;
    }

    Router consume()
    {
        wrouter_error_t err = WROUTER_OK;

        wrouter_t *router =
            wrouter_consume(&ptr_, &err);

        if (err)
            throw Error(err);

        return Router(router);
    }

private:
    wrouter_builder_t *ptr_ {};
};

class Dispatcher {
public:
    explicit Dispatcher(const Router& router)
    {
        ptr_ = wrouter_dispatcher_create(router.native());

        if (!ptr_)
            throw std::bad_alloc{};
    }

    ~Dispatcher() {
        if (ptr_)
            wrouter_dispatcher_destroy(&ptr_);
    }

    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    void dispatch(std::string_view path,
                  void *dispatch_ctx = nullptr)
    {
        wrouter_dispatch(ptr_, path.data(), dispatch_ctx);
    }

    std::unordered_map<std::string,std::string> params() const
    {
        std::unordered_map<std::string,std::string> out;

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

private:
    wrouter_dispatcher_t *ptr_ {};
};

}
