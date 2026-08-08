#pragma once

#include <memory>
#include <vector>

#include "../common/any_callable.h"
#include "../common/utils.h"
#include "../servers/engine.h"
#include "node.h"

namespace vecgui {

class Timer final : public Node {
public:
    void start_timer(float time);

    void update(double dt) override;

    void connect_signal(const std::string& signal, const AnyCallable<void>& callback) override;

    float get_remaining_time() const;

    bool is_stopped() const;

    void stop();

protected:
    void emit_timeout();

    bool is_stopped_ = true;
    double remaining_time_ = 0;
    std::vector<AnyCallable<void>> timeout_callbacks;
};

} // namespace vecgui
