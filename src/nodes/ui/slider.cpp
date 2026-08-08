#include "slider.h"

#include <optional>

#include "../../common/geometry.h"
#include "../../resources/default_resource.h"

namespace vecgui {

Slider::Slider() {
    type = NodeType::Slider;

    auto default_theme = DefaultResource::get_singleton()->get_default_theme();
}

void Slider::ready() {
    if (ready_) {
        return;
    }

    ready_ = true;

    on_ready();

    callbacks_cursor_entered.emplace_back([this] {
        hovered = true;
        InputServer::get_singleton()->set_cursor(get_window_index(), CursorShape::Hand);

        // if (theme_hovered.has_value()) {
        //     target_style_box = theme_hovered.value();
        // }
    });

    callbacks_cursor_exited.emplace_back([this] {
        hovered = false;
        InputServer::get_singleton()->set_cursor(get_window_index(), CursorShape::Arrow);

        // if (pressed) {
        //     target_style_box = theme_pressed;
        // } else {
        //     target_style_box = theme_normal;
        // }
    });
}

void Slider::input(InputEvent &event) {
    auto global_position = get_global_position();

    bool consume_flag = false;

    if (!disabled_) {
        if (event.type == InputEventType::MouseMotion) {
            auto args = event.args.mouse_motion;

            if (pressed) {
                auto local_pos = args.position - global_position;
                consume_flag = true;
                float new_ratio = (local_pos.x - grabber_margin_) / (size.x - grabber_margin_ * 2);
                change_ratio(new_ratio);
            }
        }

        if (event.type == InputEventType::MouseButton) {
            const auto args = event.args.mouse_button;

            if (!args.pressed) {
                pressed = false;
            }

            // Consumed by other UI nodes.
            if (event.consumed) {
            } else {
                if (RectF(global_position, global_position + size).contains_point(args.position)) {
                    // Mouse button pressed
                    if (args.pressed) {
                        pressed_position = args.position;
                        pressed = true;

                        auto local_pos = args.position - global_position;
                        consume_flag = true;
                        float new_ratio = (local_pos.x - grabber_margin_) / (size.x - grabber_margin_ * 2);
                        change_ratio(new_ratio);

                        // notify_pressed();
                    }

                    consume_flag = true;
                }
            }
        }
    }

    NodeUi::input(event);

    if (consume_flag) {
        event.consumed = true;
    }
}

void Slider::update(double dt) {
    NodeUi::update(dt);
}

void Slider::draw() {
    if (!visible_) {
        return;
    }

    auto vector_server = VectorServer::get_singleton();

    const auto global_position = get_global_position();

    auto default_theme = DefaultResource::get_singleton()->get_default_theme();

    vector_server->draw_line(global_position + Vec2F(grabber_margin_, size.y * 0.5),
                             global_position + Vec2F(size.x - grabber_margin_, size.y * 0.5),
                             4,
                             default_theme->slider.colors["unfilled"]);

    const float grabber_pos = ratio_ * (size.x - grabber_margin_ * 2) + grabber_margin_;

    vector_server->draw_line(global_position + Vec2F(grabber_margin_, size.y * 0.5),
                             global_position + Vec2F(grabber_pos, size.y * 0.5),
                             4,
                             default_theme->slider.colors["filled"]);

    const Vec2F grabber_center = global_position + Vec2F(grabber_pos, size.y * 0.5);

    if (pressed) {
        vector_server->draw_circle(grabber_center, 8, 0, true, default_theme->slider.colors["grabber_fill_pressed"]);
    } else {
        if (hovered) {
            vector_server->draw_circle(
                grabber_center, 8, 0, true, default_theme->slider.colors["grabber_fill_hovered"]);
        } else {
            vector_server->draw_circle(grabber_center, 8, 0, true, default_theme->slider.colors["grabber_fill"]);
        }
    }

    vector_server->draw_circle(grabber_center, 8, 3, false, default_theme->slider.colors["grabber_border"]);

    NodeUi::draw();
}

void Slider::set_position(Vec2F new_position) {
    position = new_position;
}

void Slider::connect_signal(const std::string &signal, const AnyCallable<void> &callback) {
    NodeUi::connect_signal(signal, callback);

    if (signal == "value_changed") {
        value_changed_callbacks.push_back(callback);
    }
}

void Slider::notify_value_changed(float new_value) {
    for (auto &callback : value_changed_callbacks) {
        try {
            callback.operator()<float>(std::move(new_value));
        } catch (std::bad_any_cast &) {
            Logger::error("Mismatched signal argument types in Slider::notify_value_changed!", "vecgui");
        }
    }
}

void Slider::set_range(float start, float end) {
    assert(range_end_ > range_start_);
    if (integer_mode_) {
        range_start_ = round(start);
        range_end_ = round(end);
    } else {
        range_start_ = start;
        range_end_ = end;
    }
}

float Slider::get_value() const {
    return range_start_ + (range_end_ - range_start_) * ratio_;
}

void Slider::set_value(float new_value) {
    prev_value_ = new_value;
    new_value = std::clamp(new_value, range_start_, range_end_);
    ratio_ = (new_value - range_start_) / (range_end_ - range_start_);
    notify_value_changed(new_value);
}

void Slider::set_integer_mode(bool enabled) {
    integer_mode_ = enabled;
}

void Slider::change_ratio(float new_ratio) {
    new_ratio = std::clamp(new_ratio, 0.0f, 1.0f);
    if (ratio_ == new_ratio) {
        return;
    }

    int new_value;
    if (integer_mode_) {
        new_value = round(range_start_ + (range_end_ - range_start_) * new_ratio);
        ratio_ = (new_value - range_start_) / (range_end_ - range_start_);
    } else {
        ratio_ = new_ratio;
        new_value = get_value();
    }

    if (!integer_mode_ || prev_value_ != new_value) {
        notify_value_changed(get_value());
    }
    prev_value_ = new_value;
}

} // namespace vecgui
