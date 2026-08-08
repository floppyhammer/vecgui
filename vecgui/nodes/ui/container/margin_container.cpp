#include "margin_container.h"

namespace vecgui {

void MarginContainer::calc_minimum_size() {
    Vec2F margin_size = {margin_.left + margin_.right, margin_.top + margin_.bottom};

    Vec2F max_child_min_size;
    for (auto &child : children) {
        if (child->is_ui_node()) {
            auto ui_child = dynamic_cast<NodeUi *>(child.get());
            auto child_min_size = ui_child->get_effective_minimum_size();

            max_child_min_size = max_child_min_size.max(child_min_size + margin_size);
        }
    }

    calculated_minimum_size = max_child_min_size;
}

void MarginContainer::adjust_layout() {
    // Child's local position.
    Vec2F child_position;
    child_position.x += margin_.left;
    child_position.y += margin_.top;

    auto child_size = size;
    child_size.x -= margin_.left + margin_.right;
    child_size.y -= margin_.top + margin_.bottom;

    for (auto &child : children) {
        if (child->is_ui_node()) {
            auto cast_child = dynamic_cast<NodeUi *>(child.get());
            cast_child->set_position(child_position);

            cast_child->set_size(child_size);
        }
    }
}

void MarginContainer::set_margin_all(float margin) {
    margin_ = {margin, margin, margin, margin};
}

void MarginContainer::set_margin(const RectF &margin) {
    margin_ = margin;
}

RectF MarginContainer::get_margin() const {
    return margin_;
}

} // namespace vecgui
