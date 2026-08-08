#pragma once

#include <functional>
#include <optional>

#include "../../resources/style_box.h"
#include "button/button.h"
#include "container/box_container.h"
#include "label.h"
#include "node_ui.h"
#include "texture_rect.h"

namespace vecgui {

/**
 * Numerical input field.
 */
class SpinBox : public NodeUi {
public:
    SpinBox();

    bool pressed = false;
    bool hovered = false;
    bool pressed_inside = false;
    bool drag_to_adjust_value = false;

    void input(InputEvent& event) override;

    void update(double dt) override;

    void draw() override;

    void set_position(Vec2F p_position) override;

    void set_size(Vec2F p_size) override;

    void calc_minimum_size() override;

    void connect_signal(const std::string& signal, const AnyCallable<void>& callback) override;

    void set_value(float new_value);

    float get_value() const;

protected:
    float value = 0;

    /// Only for display, the real value will not be rounded.
    uint32_t rounding_digits = 3;

    bool is_integer = false;

    bool clamped = false;

    float min_value = 0;
    float max_value = 100;

    float step = 0.1;

    bool focused = false;

    std::shared_ptr<HBoxContainer> container_h;
    std::shared_ptr<VBoxContainer> container_v;
    std::shared_ptr<Button> increase_button, decrease_button;
    std::shared_ptr<Label> label;

    std::vector<AnyCallable<void>> focused_callbacks;
    std::vector<AnyCallable<void>> value_changed_callbacks;

    std::optional<StyleBox> theme_normal, theme_focused;

protected:
    void when_focused();

    void when_value_changed();
};

} // namespace vecgui
