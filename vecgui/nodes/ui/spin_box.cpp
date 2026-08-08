#include "spin_box.h"

#include <iomanip>
#include <iostream>

#include "../../common/geometry.h"
#include "../../resources/vector_image.h"

namespace vecgui {

SpinBox::SpinBox() {
    type = NodeType::SpinBox;

    theme_normal = std::optional(StyleBox());
    theme_normal->bg_color = ColorU(10, 10, 10);

    theme_focused = std::optional(theme_normal.value());
    theme_focused->border_color = ColorU(200, 200, 200);
    theme_focused->border_width = 2;

    // Don't add the label as a child since it's not a normal node but part of the SpinBox.
    label = std::make_shared<Label>();
    label->set_mouse_filter(MouseFilter::Ignore);
    label->set_horizontal_alignment(Alignment::Center);
    label->set_vertical_alignment(Alignment::Center);
    set_value(0);

    container_v = std::make_shared<VBoxContainer>();

    container_h = std::make_shared<HBoxContainer>();
    container_h->add_child(label);
    container_h->add_child(container_v);
    container_h->set_separation(0);
    container_h->set_size(size);

    add_embedded_child(container_h);
}

void SpinBox::calc_minimum_size() {
    calculated_minimum_size = container_h->get_effective_minimum_size();
}

void SpinBox::input(InputEvent &event) {
    auto global_position = get_global_position();
    auto active_rect = RectF(global_position, global_position + size);

    bool consume_flag = false;

    if (event.type == InputEventType::MouseMotion) {
        auto args = event.args.mouse_motion;

        if (pressed_inside) {
            if (drag_to_adjust_value) {
                set_value(value + args.relative.x * step);
            }

            drag_to_adjust_value = true;

            // Capture cursor when dragging.
            // InputServer::get_singleton()->set_cursor_captured(get_window()->get_real().get(), true);
        }

        if (active_rect.contains_point(args.position)) {
            consume_flag = true;
        }
    }

    if (event.type == InputEventType::MouseButton) {
        auto args = event.args.mouse_button;

        if (!args.pressed) {
            if (pressed_inside) {
                // Release cursor when dragging ends.
                // InputServer::get_singleton()->set_cursor_captured(get_window()->get_real().get(), false);
            }

            pressed_inside = false;
            drag_to_adjust_value = false;
        }

        if (event.consumed) {
            focused = false;
            pressed_inside = false;
        } else {
            if (active_rect.contains_point(args.position)) {
                if (args.pressed) {
                    focused = true;
                    pressed_inside = true;
                }

                consume_flag = true;
            } else {
                focused = false;
            }
        }

        if (consume_flag) {
            event.consumed = true;
        }
    }

    NodeUi::input(event);
}

void SpinBox::update(double dt) {
    NodeUi::update(dt);
}

void SpinBox::draw() {
    if (!visible_) {
        return;
    }

    auto vector_server = VectorServer::get_singleton();

    auto global_position = get_global_position();

    // Draw bg.
    std::optional<StyleBox> active_style_box;
    active_style_box = focused ? theme_focused : theme_normal;

    if (active_style_box.has_value()) {
        vector_server->draw_style_box(active_style_box.value(), global_position, size);
    }

    NodeUi::draw();
}

void SpinBox::set_position(Vec2F p_position) {
    position = p_position;
}

void SpinBox::set_size(Vec2F p_size) {
    if (size == p_size) {
        return;
    }

    auto final_size = p_size.max(container_h->get_effective_minimum_size());
    final_size = final_size.max(custom_minimum_size);

    container_h->set_size(final_size);
    size = final_size;
}

void SpinBox::when_focused() {
    for (auto &callback : focused_callbacks) {
        callback();
    }
}

void SpinBox::when_value_changed() {
    for (auto &callback : value_changed_callbacks) {
        callback();
    }
}

void SpinBox::connect_signal(const std::string &signal, const AnyCallable<void> &callback) {
    if (signal == "focused") {
        focused_callbacks.push_back(callback);
    }

    if (signal == "value_changed") {
        value_changed_callbacks.push_back(callback);
    }
}

void SpinBox::set_value(float new_value) {
    if (clamped) {
        value = std::clamp(new_value, min_value, max_value);
    } else {
        value = new_value;
    }

    if (is_integer) {
        int32_t value_int = std::round(value);
        label->set_text(std::to_string(abs(value_int)));
    } else {
        std::ostringstream string_stream;
        string_stream << std::setprecision(rounding_digits) << value;
        label->set_text(string_stream.str());
    }

    when_value_changed();
}

float SpinBox::get_value() const {
    return value;
}

} // namespace vecgui
