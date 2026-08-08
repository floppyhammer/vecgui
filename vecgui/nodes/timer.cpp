#include "timer.h"

#include <string>

#include "proxy_window.h"

namespace vecgui {

void Timer::start_timer(float time) {
    if (time <= 0) {
        return;
    }
    remaining_time_ = time;
    is_stopped_ = false;
}

void Timer::update(double dt) {
    Node::update(dt);

    if (!is_stopped_ && remaining_time_ > 0) {
        remaining_time_ -= dt;
    }

    if (!is_stopped_ && remaining_time_ <= 0) {
        is_stopped_ = true;
        remaining_time_ = 0;
        emit_timeout();
    }
}

void Timer::connect_signal(const std::string& signal, const AnyCallable<void>& callback) {
    Node::connect_signal(signal, callback);

    if (signal == "timeout") {
        timeout_callbacks.emplace_back(callback);
    }
}

float Timer::get_remaining_time() const {
    return remaining_time_;
}

bool Timer::is_stopped() const {
    return is_stopped_;
}

void Timer::stop() {
    remaining_time_ = 0;
    is_stopped_ = true;
}

void Timer::emit_timeout() {
    for (auto& c : timeout_callbacks) {
        c();
    }
}

} // namespace vecgui
