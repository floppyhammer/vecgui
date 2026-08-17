#include "node_ui.h"

#include "../../common/context.h"
#include "../../common/geometry.h"
#include "../../resources/default_resource.h"
#include "../../servers/vector_server.h"
#include "../scene_tree.h"

using Pathfinder::Rect;

namespace vecgui {

NodeUi::NodeUi() {
    type = NodeType::NodeUi;
    calculated_global_transform = Pathfinder::Transform2();
}

void NodeUi::calc_minimum_size() {
    calculated_minimum_size = {};
}

void NodeUi::adjust_layout() {
    size = get_effective_minimum_size().max(size);
}

void NodeUi::queue_relayout() {
    if (layout_is_dirty) {
        return;
    }

    layout_is_dirty = true;

    if (parent && parent->is_ui_node()) {
        auto ui_parent = dynamic_cast<NodeUi *>(parent);
        ui_parent->queue_relayout();
    }
}

Vec2F NodeUi::get_effective_minimum_size() const {
    // Take both custom_minimum_size and calculated_minimum_size into account.
    return custom_minimum_size.max(calculated_minimum_size);
}

void NodeUi::draw() {
    Node::draw();

    if (visible_ && debug_box.has_value()) {
        auto context = get_context();
        if (!context) {
            return;
        }

        auto vector_server = context->vector_server;

        auto global_transform = get_global_transform();

        vector_server->draw_style_box(debug_box.value(), global_transform, size);
    }
}

void NodeUi::update(double dt) {
    Node::update(dt);
}

void NodeUi::input(InputEvent &event) {
    if (mouse_filter == MouseFilter::Ignore) {
        return;
    }

    auto global_to_local = calculated_global_transform.inverse();

    // Hit testing rect in local space.
    auto local_rect = RectF(-pivot * size, -pivot * size + size);

    // Handle mouse input propagation.
    bool consume_flag = false;

    switch (event.type) {
        case InputEventType::MouseMotion: {
            auto local_pos = global_to_local * event.args.mouse_motion.position;
            // Mouse position relative to the node's top-left.
            local_mouse_position = local_pos + pivot * size;

            if (local_rect.contains_point(local_pos)) {
                if (!event.consumed) {
                    if (!is_cursor_inside) {
                        cursor_entered();
                    }
                    is_cursor_inside = true;
                }

                consume_flag = true;
            } else {
                if (is_cursor_inside) {
                    is_cursor_inside = false;
                    cursor_exited();
                }
            }
        } break;
        case InputEventType::MouseButton: {
            auto args = event.args.mouse_button;
            auto local_pos = global_to_local * args.position;

            if (!event.consumed && args.pressed) {
                if (local_rect.contains_point(local_pos)) {
                    grab_focus();

                    consume_flag = true;
                } else {
                    release_focus();
                }
            }
        } break;
        default:
            break;
    }

    if (mouse_filter == MouseFilter::Stop && consume_flag) {
        event.consumed = true;
    }

    Node::input(event);
}

Vec2F NodeUi::get_global_position() const {
    return calculated_global_position;
}

Vec2F NodeUi::get_draw_position() const {
    return calculated_global_position - pivot * size;
}

Transform2 NodeUi::get_local_transform() const {
    // Correct order for local transform:
    // T = Translation(position) * Rotation(rotation) * Scale(scale) * Translation(-pivot * size)
    // In Pathfinder API, A.op(B) means Op(B) * A.
    // So we start from the innermost (last applied) operation.
    return Transform2::from_translation(-pivot * size).scale(scale).rotate(rotation).translate(position);
}

Transform2 NodeUi::get_global_transform() const {
    return calculated_global_transform;
}

void NodeUi::calc_global_transform(const Transform2 &parent_global_transform) {
    calculated_global_transform = parent_global_transform * get_local_transform();
    calculated_global_position = position;
    if (parent && parent->is_ui_node()) {
        auto ui_parent = dynamic_cast<NodeUi *>(parent);
        calculated_global_position = ui_parent->get_global_position() + position;
    }
}

void NodeUi::set_mouse_filter(MouseFilter filter) {
    mouse_filter = filter;
}

void NodeUi::set_pivot(Vec2F new_pivot) {
    pivot = new_pivot;
}

Vec2F NodeUi::get_pivot() const {
    return pivot;
}

void NodeUi::set_rotation(float new_rotation) {
    rotation = new_rotation;
}

float NodeUi::get_rotation() const {
    return rotation;
}

void NodeUi::set_rotation_degree(float degree) {
    rotation = degree * (Pathfinder::PI / 180.0f);
}

float NodeUi::get_rotation_degree() const {
    return rotation * (180.0f / Pathfinder::PI);
}

void NodeUi::set_scale(Vec2F new_scale) {
    scale = new_scale;
}

Vec2F NodeUi::get_scale() const {
    return scale;
}

void NodeUi::set_position(Vec2F new_position) {
    position = new_position;
}

void NodeUi::set_size(Vec2F new_size) {
    if (size == new_size) {
        return;
    }

    size = new_size;
    queue_relayout();

    // When size changes, all UI children (including embedded ones) must be notified
    // to re-evaluate their anchors and internal layouts.
    for (auto &child : get_all_children()) {
        if (child->is_ui_node()) {
            auto cast_child = dynamic_cast<NodeUi *>(child.get());
            cast_child->queue_relayout();
        }
    }
}

Vec2F NodeUi::get_position() const {
    return position;
}

Vec2F NodeUi::get_size() const {
    return size;
}

void NodeUi::set_custom_minimum_size(Vec2F new_size) {
    if (custom_minimum_size == new_size) {
        return;
    }
    custom_minimum_size = new_size;
    queue_relayout();
}

Vec2F NodeUi::get_custom_minimum_size() const {
    return custom_minimum_size;
}

Vec2F NodeUi::get_local_mouse_position() const {
    return local_mouse_position;
}

void NodeUi::grab_focus() {
    focused = true;
}

void NodeUi::release_focus() {
    for (auto &callback : callbacks_focus_released) {
        try {
            callback();
        } catch (std::bad_any_cast &) {
            Logger::error("Mismatched signal argument types!");
        }
    }

    focused = false;
}

ColorU NodeUi::get_global_modulate() {
    if (parent && parent->is_ui_node()) {
        auto cast_parent = dynamic_cast<NodeUi *>(parent);
        return ColorU(modulate.to_f32() * cast_parent->get_global_modulate().to_f32());
    }

    return ColorU::white();
}

bool NodeUi::is_inside_container() const {
    if (parent) {
        switch (parent->get_node_type()) {
            case NodeType::Container:
            case NodeType::CenterContainer:
            case NodeType::CollapseContainer:
            case NodeType::MarginContainer:
            case NodeType::HBoxContainer:
            case NodeType::VBoxContainer:
            case NodeType::ScrollContainer:
            case NodeType::TabContainer: {
                return true;
            }
            default:
                return false;
        }
    }
    return false;
}

Vec2F NodeUi::get_max_child_min_size() const {
    Vec2F max_child_min_size;

    for (auto &child : children) {
        if (child->is_ui_node()) {
            auto cast_child = dynamic_cast<NodeUi *>(child.get());
            max_child_min_size = max_child_min_size.max(cast_child->get_effective_minimum_size());
        }
    }

    return max_child_min_size;
}

void NodeUi::apply_anchor() {
    if (is_inside_container()) {
        return;
    }

    Vec2F parent_size;

    // If it has no parent or the parent is not a UI node, use the parent window's size for anchoring.
    if (parent && parent->is_ui_node()) {
        auto ui_parent = dynamic_cast<NodeUi *>(parent);
        parent_size = ui_parent->get_size();
    } else if (tree_) {
        parent_size = tree_->get_view_size().to_f32();
    }

    auto minimum_size = get_effective_minimum_size();

    float center_x = (parent_size.x - minimum_size.x) * 0.5f;
    float center_y = (parent_size.y - minimum_size.y) * 0.5f;
    float right = parent_size.x - minimum_size.x;
    float bottom = parent_size.y - minimum_size.y;

    // Always shrink to minimal size when anchor is set.
    if (anchor_mode != AnchorFlag::None || size < minimum_size) {
        set_size(minimum_size);
    }

    switch (anchor_mode) {
        case AnchorFlag::None: {
            // Do nothing.
        } break;
        case AnchorFlag::TopLeft: {
            position = {0, 0};
        } break;
        case AnchorFlag::TopRight: {
            position = {right, 0};
        } break;
        case AnchorFlag::BottomRight: {
            position = {right, bottom};
        } break;
        case AnchorFlag::BottomLeft: {
            position = {0, bottom};
        } break;
        case AnchorFlag::CenterLeft: {
            position = {0, center_y};
        } break;
        case AnchorFlag::CenterRight: {
            position = {right, center_y};
        } break;
        case AnchorFlag::CenterTop: {
            position = {center_x, 0};
        } break;
        case AnchorFlag::CenterBottom: {
            position = {center_x, bottom};
        } break;
        case AnchorFlag::Center: {
            position = {center_x, center_y};
        } break;
        case AnchorFlag::LeftWide: {
            position = {0, 0};
            size = {minimum_size.x, parent_size.y};
        } break;
        case AnchorFlag::RightWide: {
            position = {right, 0};
            size = {minimum_size.x, parent_size.y};
        } break;
        case AnchorFlag::TopWide: {
            position = {0, 0};
            size = {parent_size.x, minimum_size.y};
        } break;
        case AnchorFlag::BottomWide: {
            position = {0, bottom};
            size = {parent_size.x, minimum_size.y};
        } break;
        case AnchorFlag::VCenterWide: {
            position = {0, center_y};
            size = {parent_size.x, minimum_size.y};
        } break;
        case AnchorFlag::HCenterWide: {
            position = {center_x, 0};
            size = {minimum_size.x, parent_size.y};
        } break;
        case AnchorFlag::FullRect: {
            position = {0, 0};
            size = {parent_size.x, parent_size.y};
        } break;
        case AnchorFlag::Max: {
            abort();
        }
    }
}

void NodeUi::cursor_entered() {
    for (auto &callback : callbacks_cursor_entered) {
        callback();
    }
}

void NodeUi::cursor_exited() {
    for (auto &callback : callbacks_cursor_exited) {
        callback();
    }
}

void NodeUi::set_anchor_flag(AnchorFlag anchor_flag) {
    if (anchor_flag == anchor_mode) {
        return;
    }

    anchor_mode = anchor_flag;
    queue_relayout();
}

AnchorFlag NodeUi::get_anchor_flag() const {
    return anchor_mode;
}

void NodeUi::when_parent_size_changed(Vec2F new_size) {
    queue_relayout();
    for (auto &child : get_all_children()) {
        if (child->is_ui_node()) {
            auto cast_child = dynamic_cast<NodeUi *>(child.get());
            cast_child->when_parent_size_changed(size);
        }
    }
}

void NodeUi::connect_signal(const std::string &signal, const AnyCallable<void> &callback) {
    Node::connect_signal(signal, callback);

    if (signal == "focus_released") {
        callbacks_focus_released.push_back(callback);
    }
}

} // namespace vecgui
