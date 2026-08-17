#include "split_container.h"

#include <string>

#include "../../../common/utils.h"
#include "../../../resources/default_resource.h"
#include "../button/check_button.h"

namespace vecgui {

SplitContainer::SplitContainer() {
    type = NodeType::SplitContainer;

    container_sizing.flag_h = ContainerSizingFlag::Fill;
}

void SplitContainer::adjust_layout() {
    // Calculate grabber position first.
    int valid_count = 0;

    for (const auto &child : children) {
        if (valid_count > 1) {
            break;
        }

        if (child->is_ui_node() && child->get_visibility()) {
            auto cast_child = dynamic_cast<NodeUi *>(child.get());
            auto child_min_size = cast_child->get_effective_minimum_size();

            if (valid_count == 0) {
                split_to_right_length =
                    std::min(size.x - (child_min_size.x + grabber_size_ * 0.5f), split_to_right_length);
            } else {
                split_to_right_length = std::max(child_min_size.x + grabber_size_ * 0.5f, split_to_right_length);
            }

            valid_count++;
        }
    }

    splitting_enabled_ = valid_count == 2;

    float effective_grabber_size = grabber_size_;

    if (!splitting_enabled_) {
        split_to_right_length = 0;
        effective_grabber_size = 0;
    }

    // Adjust child size.
    valid_count = 0;

    for (const auto &child : children) {
        if (valid_count > 1) {
            break;
        }

        if (child->is_ui_node() && child->get_visibility()) {
            auto cast_child = dynamic_cast<NodeUi *>(child.get());

            float grabber_left_edge_pos = size.x - split_to_right_length - effective_grabber_size * 0.5f;

            Vec2F child_size;
            Vec2F child_pos;
            if (valid_count == 0) {
                child_size = {grabber_left_edge_pos, size.y};
                child_pos = {0, 0};
            } else {
                child_size = {size.x - grabber_left_edge_pos - grabber_size_, size.y};
                child_pos = {grabber_left_edge_pos + grabber_size_, 0};
            }
            cast_child->set_position(child_pos);
            cast_child->set_size(child_size);

            valid_count++;
        }
    }
}

void SplitContainer::connect_signal(const std::string &signal, const AnyCallable<void> &callback) {
    Container::connect_signal(signal, callback);

    if (signal == "grabber_moved") {
        grabber_moved_callbacks.push_back(callback);
    }
}

void SplitContainer::when_grabber_moved() {
    for (auto &callback : grabber_moved_callbacks) {
        try {
            callback();
        } catch (std::bad_any_cast &) {
            Logger::error("Mismatched signal argument types!", "vecgui");
        }
    }
}

void SplitContainer::calc_minimum_size() {
    // Get the minimum child size.
    Vec2F min_size{};
    int valid_count = 0;

    for (const auto &child : children) {
        if (child->is_ui_node() && child->get_visibility() && valid_count < 2) {
            auto cast_child = dynamic_cast<NodeUi *>(child.get());
            auto child_min_size = cast_child->get_effective_minimum_size();
            min_size += child_min_size;
        }
        valid_count++;
    }
    min_size.x += grabber_size_;

    calculated_minimum_size = min_size;
}

void SplitContainer::update(const double dt) {
    Container::update(dt);

    adjust_layout();
}

void SplitContainer::input(InputEvent &event) {
    const auto global_position = get_global_position();

    bool consume_flag = false;

    if (splitting_enabled_) {
        float grabber_left_edge_pos = size.x - split_to_right_length - grabber_size_ * 0.5f;

        if (event.type == InputEventType::MouseMotion) {
            auto args = event.args.mouse_motion;

            auto context = get_context();
            if (!event.consumed && context) {
                if (RectF(global_position + Vec2F(grabber_left_edge_pos, 0),
                          global_position + Vec2F(grabber_left_edge_pos, 0) + Vec2F(grabber_size_, size.y))
                        .contains_point(args.position)) {
                    cursor_in_ = true;
                    context->input_server->set_cursor(context->render_context, get_window_index(), CursorShape::ResizeH);
                } else {
                    if (cursor_in_) {
                        context->input_server->set_cursor(context->render_context, get_window_index(), CursorShape::Arrow);
                    }
                    cursor_in_ = false;
                }
            } else {
                if (cursor_in_ && context) {
                    context->input_server->set_cursor(context->render_context, get_window_index(), CursorShape::Arrow);
                }
                cursor_in_ = false;
            }

            if (grabber_pressed_pos_.has_value()) {
                split_to_right_length =
                    size.x - (args.position.x - global_position.x - grabber_pressed_offset.x + grabber_size_ * 0.5f);
            }
        }

        if (event.type == InputEventType::MouseButton) {
            const auto args = event.args.mouse_button;

            if (!args.pressed) {
                if (grabber_pressed_pos_.has_value()) {
                    grabber_pressed_pos_ = {};
                }
            }

            // Consumed by other UI nodes.
            if (!event.consumed) {
                if (args.pressed) {
                    if (RectF(global_position + Vec2F(grabber_left_edge_pos, 0),
                              global_position + Vec2F(grabber_left_edge_pos, 0) + Vec2F(grabber_size_, size.y))
                            .contains_point(args.position)) {
                        grabber_pressed_pos_ = args.position;
                        grabber_pressed_offset = args.position - global_position - Vec2F(grabber_left_edge_pos, 0);
                    }
                }
                consume_flag = true;
            }
        }
    }

    NodeUi::input(event);

    if (consume_flag) {
        event.consumed = true;
    }
}

void SplitContainer::draw() {
    if (!visible_) {
        return;
    }

    auto context = get_context();
    if (!context) {
        return;
    }

    const auto vector_server = context->vector_server;

    const auto global_position = get_global_position();

    if (splitting_enabled_) {
        const float grabber_left_edge_pos = size.x - split_to_right_length - grabber_size_ * 0.5f;

        if (grabber_pressed_pos_.has_value()) {
            const auto start = global_position + Vec2F(grabber_left_edge_pos + grabber_size_ * 0.5f, size.y * 0.25f);
            const auto end = start + Vec2F(0, size.y * 0.5f);

            const auto default_theme = context->default_resource->get_default_theme();
            vector_server->draw_line(start, end, 2, default_theme->accent_color);
        }
    }
}

void SplitContainer::set_separation(const float new_separation) {
    grabber_size_ = new_separation;
}

void SplitContainer::set_split_ratio(const float new_ratio) {
    split_to_right_length = (1.0f - new_ratio) * size.x;
}

} // namespace vecgui
