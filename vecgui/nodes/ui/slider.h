#pragma once

#include <functional>

#include "../../resources/style_box.h"
#include "container/box_container.h"
#include "container/margin_container.h"
#include "label.h"
#include "node_ui.h"
#include "texture_rect.h"

namespace vecgui {

class Slider : public NodeUi {
public:
    Slider();

    void ready() override;

    void input(InputEvent &event) override;

    void update(double dt) override;

    void draw() override;

    void set_position(Vec2F _position) override;

    void connect_signal(const std::string &signal, const AnyCallable<void> &callback) override;

    void notify_value_changed(float new_value);

    void set_range(float start, float end);

    float get_value() const;

    void set_value(float new_value);

    void set_integer_mode(bool enabled);

protected:
    bool pressed = false;
    std::optional<Vec2F> pressed_position;
    bool hovered = false;

    bool disabled_ = false;

    float ratio_ = 0.5f;
    float prev_value_ = 0.f;

    float range_start_ = 0;
    float range_end_ = 100;

    bool integer_mode_ = false;

    float grabber_margin_ = 4.f;

    std::vector<AnyCallable<void>> value_changed_callbacks;

    void change_ratio(float new_ratio);
};

} // namespace vecgui
