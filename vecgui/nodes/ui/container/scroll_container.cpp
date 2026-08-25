#include "scroll_container.h"

#include "../../../scene_tree.h"
#include "../../../servers/render_server.h"

using Pathfinder::clamp;

namespace vecgui {

ScrollContainer::ScrollContainer() {
    type = NodeType::ScrollContainer;

    theme_scroll_bar.bg_color = ColorU(100, 100, 100, 0);
    theme_scroll_bar.corner_radius = 0;

    theme_scroll_grabber.bg_color = ColorU(163, 163, 163, 150);
    theme_scroll_grabber.corner_radius = 8;
}

void ScrollContainer::adjust_layout() {
    if (children.empty()) {
        return;
    }

    Vec2F max_child_min_size = get_max_child_min_size();

    auto min_size = max_child_min_size.max(custom_minimum_size);

    // Adjust own size.

    if (!vscroll_enabled) {
        size.y = size.max(min_size).y;
    }
    if (!hscroll_enabled) {
        size.x = size.max(min_size).x;
    }

    // Adjust child size.
    for (auto &child : children) {
        if (child->is_ui_node()) {
            auto cast_child = dynamic_cast<NodeUi *>(child.get());

            if (!vscroll_enabled) {
                cast_child->set_size({cast_child->get_size().x, size.y});
            }
            if (!hscroll_enabled) {
                cast_child->set_size({size.x, cast_child->get_size().y});
            }
        }
    }
}

void ScrollContainer::calc_minimum_size() {
    // Get the minimum child size.
    Vec2F min_child_size{};
    for (const auto &child : children) {
        if (!child->is_ui_node()) {
            continue;
        }
        auto cast_child = dynamic_cast<NodeUi *>(child.get());
        auto child_min_size = cast_child->get_effective_minimum_size();
        min_child_size = min_child_size.max(child_min_size);
    }

    calculated_minimum_size = {};
    if (!hscroll_enabled) {
        calculated_minimum_size.x = min_child_size.x;
    }
    if (!vscroll_enabled) {
        calculated_minimum_size.y = min_child_size.y;
    }
}

void ScrollContainer::input(InputEvent &event) {
    auto global_position = get_global_position();

    auto active_rect = RectF(global_position, global_position + size);

    // Handle mouse input propagation.
    bool consume_flag = false;

    switch (event.type) {
        case InputEventType::MouseScroll: {
            float delta = event.args.mouse_scroll.y_delta;

            auto context = get_context();
            if (context && active_rect.contains_point(context->input_server->cursor_position)) {
                if (!event.consumed) {
                    if (hscroll_enabled && !vscroll_enabled) {
                        hscroll -= delta * scroll_speed;
                    } else {
                        if (context->input_server->is_key_pressed(KeyCode::LeftShift)) {
                            hscroll -= delta * scroll_speed;
                        } else {
                            vscroll -= delta * scroll_speed;
                        }
                    }
                }

                // Will stop input propagation.
                if (mouse_filter == MouseFilter::Stop) {
                    consume_flag = true;
                }
            }
        } break;
        case InputEventType::MouseButton: {
            if (event.args.mouse_button.button == 0) {
                const auto args = event.args.mouse_button;
                if (args.pressed) {
                    if (!event.consumed) {
                        if (active_rect.contains_point(args.position)) {
                            pressed_mouse_position = args.position;
                            prev_hscroll = hscroll;
                            prev_vscroll = vscroll;
                        }
                        consume_flag = true;
                    }
                } else {
                    if (max_movement.length() > 1) {
                        inertial_speed = max_movement * inertia;
                        lerp_elapsed_ = 0;
                    }

                    pressed_mouse_position.reset();
                    prev_hscroll.reset();
                    prev_hscroll.reset();
                }
            }
        } break;
        case InputEventType::MouseMotion: {
            const auto args = event.args.mouse_motion;

            max_movement = args.relative;

            if (pressed_mouse_position.has_value()) {
                hscroll = prev_hscroll.value() - (args.position.x - pressed_mouse_position.value().x);
                vscroll = prev_vscroll.value() - (args.position.y - pressed_mouse_position.value().y);
            }
        } break;
        default:
            break;
    }

    NodeUi::input(event);
}

void ScrollContainer::update(double dt) {
    NodeUi::update(dt);

    if (children.empty() || !children.front()->is_ui_node()) {
        return;
    }

    if (inertial_speed.length() > 0) {
        lerp_elapsed_ += dt;
        float t = std::clamp(lerp_elapsed_ / lerp_duration_, 0.0f, 1.0f);
        inertial_speed.x = Pathfinder::lerp(inertial_speed.x, 0, t);
        inertial_speed.y = Pathfinder::lerp(inertial_speed.y, 0, t);

        vscroll -= inertial_speed.y * dt;
        hscroll -= inertial_speed.x * dt;
    }

    vscroll = std::max(0.0f, vscroll);
    hscroll = std::max(0.0f, hscroll);

    Vec2F max_child_min_size = get_max_child_min_size();

    if (max_child_min_size.x > size.x) {
        hscroll = std::min(hscroll, max_child_min_size.x - size.x);
    } else {
        hscroll = 0;
    }

    if (max_child_min_size.y > size.y) {
        vscroll = std::min(vscroll, max_child_min_size.y - size.y);
    } else {
        vscroll = 0;
    }

    // Scroll container can only have one effective control child.
    auto content = (NodeUi *)children.front().get();

    content->set_position({-hscroll, -vscroll});
}

void ScrollContainer::draw_scroll_bar() {
    if (children.empty() || !children.front()->is_ui_node()) {
        return;
    }

    if (!visible_) {
        return;
    }

    // Scroll container can only have one effective control child.
    auto content = (NodeUi *)children.front().get();
    auto content_size = content->get_size();

    auto context = get_context();
    if (!context) {
        return;
    }
    auto vector_server = context->vector_server;

    auto global_pos = get_global_position();
    auto size = get_size();

    float bar_width = 4.0;

    // Vertical.
    if (content_size.y > size.y) {
        auto scroll_bar_pos = Vec2F(size.x - bar_width, 0) + global_pos;
        auto scroll_bar_size = Vec2F(bar_width, size.y);

        vector_server->draw_style_box(theme_scroll_bar, scroll_bar_pos, scroll_bar_size);

        auto grabber_length = size.y / content_size.y * size.y;

        auto grabber_pos = Vec2F(size.x - bar_width, size.y / content_size.y * vscroll) + global_pos;
        auto grabber_size = Vec2F(bar_width, grabber_length);

        vector_server->draw_style_box(theme_scroll_grabber, grabber_pos, grabber_size);
    }

    // Horizontal.
    if (content_size.x > size.x) {
        auto scroll_bar_pos = Vec2F(0, size.y - bar_width) + global_pos;
        auto scroll_bar_size = Vec2F(size.x, bar_width);

        vector_server->draw_style_box(theme_scroll_bar, scroll_bar_pos, scroll_bar_size);

        auto grabber_length = size.x / content_size.x * size.x;

        auto grabber_pos = Vec2F(size.x / content_size.x * hscroll, size.y - bar_width) + global_pos;
        auto grabber_size = Vec2F(grabber_length, bar_width);

        vector_server->draw_style_box(theme_scroll_grabber, grabber_pos, grabber_size);
    }
}

void ScrollContainer::set_hscroll(int32_t value) {
    if (children.empty()) {
        return;
    }

    hscroll = value;
}

int32_t ScrollContainer::get_hscroll() const {
    return hscroll;
}

void ScrollContainer::set_vscroll(int32_t value) {
    if (children.empty()) {
        return;
    }

    vscroll = value;
}

int32_t ScrollContainer::get_vscroll() const {
    return vscroll;
}

void ScrollContainer::enable_hscroll(bool enabled) {
    hscroll_enabled = enabled;
}

void ScrollContainer::enable_vscroll(bool enabled) {
    vscroll_enabled = enabled;
}

void ScrollContainer::set_size(Vec2F new_size) {
    size = new_size;
}

void ScrollContainer::pre_draw_children() {
    if (!visible_) {
        return;
    }

    if (get_size().is_any_zero()) {
        return;
    }

    float dpi_scale = tree_ ? tree_->get_dpi_scale() : 1.0f;

    auto context = get_context();
    if (!context) {
        return;
    }

#ifdef VECGUI_USE_WINDOW
    auto window_builder = context->render_context->get_window_builder();
    if (window_builder) {
        dpi_scale = window_builder->get_dpi_scaling_factor(get_window_index());
    }
#endif

    auto global_pos = get_global_position();

    auto size = get_size() * dpi_scale;

    auto vector_server = context->vector_server;

    auto canvas = vector_server->get_canvas();

    // Use a RenderTarget to achieve content clip, instead of using a clip path.
    Pathfinder::RenderTargetDesc render_target_desc = {size.to_i32(), "ScrollContainer render target"};

    // Draw all children onto the temporary render target.
    temp_draw_data.render_target_id = canvas->get_scene()->push_render_target(render_target_desc);

    // For the temporary render target, we need to offset all child nodes back to the origin.
    vector_server->global_transform_offset = Transform2::from_translation(-global_pos);
}

void ScrollContainer::post_draw_children() {
    if (!visible_ || get_size().is_any_zero()) {
        return;
    }

    auto global_pos = get_global_position();
    auto size = get_size();

    auto context = get_context();
    if (!context) {
        return;
    }

    auto vector_server = context->vector_server;

    auto canvas = vector_server->get_canvas();

    // Don't draw on the temporary render target anymore.
    canvas->get_scene()->pop_render_target();

    float dpi_scale = tree_ ? tree_->get_dpi_scale() : 1.0f;

#ifdef VECGUI_USE_WINDOW
    auto window_builder = context->render_context->get_window_builder();
    if (window_builder) {
        dpi_scale = window_builder->get_dpi_scaling_factor(get_window_index());
    }
#endif

    auto dst_rect = RectF(global_pos * dpi_scale, (global_pos + size) * dpi_scale);
    canvas->draw_render_target(temp_draw_data.render_target_id, dst_rect);

    vector_server->global_transform_offset = Transform2();
    draw_scroll_bar();
}

} // namespace vecgui
