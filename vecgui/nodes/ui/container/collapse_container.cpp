#include "collapse_container.h"

#include <string>

#include "../../../common/utils.h"
#include "../../../resources/default_resource.h"
#include "../button/check_button.h"

namespace vecgui {

CollapseContainer::CollapseContainer(CollapseButtonType button_type) : button_type_(button_type) {
    type = NodeType::CollapseContainer;

    set_color(ColorU(78, 135, 82));

    container_sizing.flag_h = ContainerSizingFlag::Fill;
}

void CollapseContainer::on_ready() {
    NodeUi::on_ready();

    if (collapse_button_) {
        return;
    }

    auto context = get_context();
    if (!context) {
        return;
    }

    switch (button_type_) {
        case CollapseButtonType::Check: {
            collapse_button_ = std::make_shared<CheckButton>();
            collapse_button_->set_icon_normal(
                std::make_shared<VectorImage>(context, get_asset_dir("icons/toggle_off.svg"), true));
            collapse_button_->set_icon_pressed(
                std::make_shared<VectorImage>(context, get_asset_dir("icons/toggle_on.svg"), true));
        } break;
        default: {
            collapse_button_ = std::make_shared<Button>();
        } break;
    }

    collapse_button_->set_custom_minimum_size({0, title_bar_height_});
    collapse_button_->set_text(title_);
    collapse_button_->set_flat(true);
    collapse_button_->set_toggle_mode(true);
    collapse_button_->connect_signal("toggled", [this](bool p_pressed) { set_collapse(!p_pressed); });
    collapse_button_->set_toggled_no_signal(!collapsed_);

    add_embedded_child(collapse_button_);
}

void CollapseContainer::adjust_layout() {
    // Get the minimum size.
    auto min_size = get_effective_minimum_size();

    // Adjust own size.
    size = size.max(min_size);

    if (!is_inside_container() && collapsed_) {
        size = min_size;
    }

    if (collapse_button_) {
        collapse_button_->set_size({size.x, title_bar_height_});
    }

    // Adjust child size.
    for (auto &child : children) {
        if (child->is_ui_node()) {
            auto child_size = size;
            child_size -= Vec2F{margin_ * 2, margin_ * 2 + title_bar_height_};
            auto cast_child = dynamic_cast<NodeUi *>(child.get());
            cast_child->set_position({margin_, title_bar_height_ + margin_});
            cast_child->set_size(child_size);
        }

        child->set_visibility(!collapsed_);
    }
}

void CollapseContainer::set_color(ColorU color) {
    theme_color_ = color;
}

void CollapseContainer::set_collapse(bool collapse) {
    if (collapsed_ == collapse) {
        return;
    }
    collapsed_ = collapse;
    if (collapse_button_) {
        collapse_button_->set_toggled_no_signal(!collapse);
    }

    when_collapsed(collapsed_);

    if (!collapse) {
        this->size = this->size_before_collapse_;
    }

    if (collapse) {
        this->size_before_collapse_ = this->size;
    }
}

bool CollapseContainer::get_collapse() const {
    return collapsed_;
}

void CollapseContainer::when_collapsed(bool collapsed) {
    for (auto &callback : collapsed_callbacks) {
        try {
            callback.operator()<bool>(std::move(collapsed));
        } catch (std::bad_any_cast &) {
            Logger::error("Mismatched signal argument types!", "vecgui");
        }
    }
}

void CollapseContainer::connect_signal(const std::string &signal, const AnyCallable<void> &callback) {
    Container::connect_signal(signal, callback);

    if (signal == "collapsed") {
        collapsed_callbacks.push_back(callback);
    }
}

void CollapseContainer::calc_minimum_size() {
    // Get the minimum child size.
    Vec2F min_child_size{};
    if (!collapsed_) {
        for (const auto &child : children) {
            if (child->is_ui_node()) {
                auto cast_child = dynamic_cast<NodeUi *>(child.get());
                auto child_min_size = cast_child->get_effective_minimum_size();
                min_child_size = min_child_size.max(child_min_size);
            }
        }

        min_child_size += Vec2F{margin_, margin_} * 2;
    }

    Vec2F min_embeded_child_size{};
    for (const auto &child : embedded_children) {
        if (child->is_ui_node()) {
            auto cast_child = dynamic_cast<NodeUi *>(child.get());
            auto child_min_size = cast_child->get_effective_minimum_size();
            min_embeded_child_size = min_embeded_child_size.max(child_min_size);
        }
    }

    calculated_minimum_size.x = std::max(min_child_size.x, min_embeded_child_size.x);
    calculated_minimum_size.y = min_child_size.y + min_embeded_child_size.y;
}

void CollapseContainer::set_title(std::string title) {
    title_ = title;
    if (collapse_button_) {
        collapse_button_->set_text(title);
    }
}

void CollapseContainer::update(double dt) {
    NodeUi::update(dt);

    adjust_layout();
}

void CollapseContainer::draw() {
    if (!visible_) {
        return;
    }

    auto context = get_context();
    if (!context) {
        return;
    }

    auto vector_server = context->vector_server;

    auto global_position = get_global_position();

    auto default_theme = context->default_resource->get_default_theme();

    auto theme_title_bar = theme_override_title_bar_.value_or(default_theme->collapse_container.styles["title_bar"]);
    theme_title_bar.border_width = 2;
    theme_title_bar.corner_radii = {8, 8, 0, 0};
    auto theme_bg = theme_override_bg_.value_or(default_theme->collapse_container.styles["background"]);

    if (collapsed_) {
        theme_title_bar.bg_color = theme_bg.bg_color;
        theme_title_bar.corner_radii = {8, 8, 8, 8};
    } else {
        theme_title_bar.bg_color = theme_color_;
        theme_title_bar.corner_radii = {8, 8, 0, 0};
    }

    theme_title_bar.border_color = theme_color_;
    theme_bg.border_color = theme_color_;

    if (!collapsed_) {
        vector_server->draw_style_box(theme_bg, global_position, size);
    }

    auto title_bar_size = size;
    title_bar_size.y = title_bar_height_;
    vector_server->draw_style_box(theme_title_bar, global_position, title_bar_size);
}

} // namespace vecgui
