#pragma once

#include <any>
#include <functional>

namespace vecgui {

template <typename Ret>
struct AnyCallable {
    AnyCallable() = default;
    template <typename F>
    AnyCallable(F&& fun) : AnyCallable(std::function(std::forward<F>(fun))) {
    }
    template <typename... Args>
    AnyCallable(std::function<Ret(Args...)> fun) : m_any(fun) {
    }
    template <typename... Args>
    Ret operator()(Args&&... args) {
        return std::invoke(std::any_cast<std::function<Ret(Args...)>>(m_any), std::forward<Args>(args)...);
    }

    std::any m_any;
};

} // namespace vecgui
