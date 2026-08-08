#include "tab_container.h"

#include "../../../resources/default_resource.h"
#include "scroll_container.h"

namespace vecgui {

TabContainer::TabContainer() {
    type = NodeType::TabContainer;

    auto default_theme = DefaultResource::get_singleton()->get_default_theme();

    button_scroll_container = std::make_shared<ScrollContainer>();
    button_scroll_container->enable_vscroll(false);
    add_embedded_child(button_scroll_container);

    button_container = std::make_shared<HBoxContainer>();
    button_scroll_container->add_child(button_container);

    tab_button_group = std::make_shared<ToggleButtonGroup>();
}

void TabContainer::adjust_layout() {
    // Ensure self satisfies minimum size constraints.
    size = get_effective_minimum_size().max(size);

    auto tab_button_height = button_container->get_effective_minimum_size().y;

    // Key fix: explicitly assign position and size to the embedded tab bar container.
    button_scroll_container->set_position({0, 0});
    button_scroll_container->set_size({size.x, tab_button_height});

    for (int i = 0; i < (int)children.size(); i++) {
        if (!children[i]->is_ui_node()) {
            continue;
        }

        // Set visibility only when state changes to avoid relayout loops.
        bool should_be_visible = (current_tab.has_value() && (uint32_t)i == current_tab.value());
        if (children[i]->get_visibility() != should_be_visible) {
            children[i]->set_visibility(should_be_visible);
        }

        auto ui_child = dynamic_cast<NodeUi *>(children[i].get());
        ui_child->set_position({0, tab_button_height});
        ui_child->set_size({size.x, size.y - tab_button_height});
    }
}

void TabContainer::update(double dt) {
    Container::update(dt);
}

void TabContainer::calc_minimum_size() {
    Vec2F min_size;

    // Find the largest child size.
    for (auto &child : children) {
        // We should take account of invisible UI nodes.
        if (!child->is_ui_node()) {
            continue;
        }

        auto cast_child = dynamic_cast<NodeUi *>(child.get());
        auto child_min_size = cast_child->get_effective_minimum_size();

        min_size = min_size.max(child_min_size);
    }

    auto tab_button_height = button_container->get_effective_minimum_size().y;
    min_size.y += tab_button_height;

    calculated_minimum_size = min_size;
}

std::optional<uint32_t> TabContainer::get_current_tab() const {
    return current_tab;
}

void TabContainer::set_current_tab(uint32_t index) {
    current_tab = index;
    tab_buttons[index]->set_toggled(true);
}

void TabContainer::draw() {
    Container::draw();

    auto vs = VectorServer::get_singleton();

    auto global_pos = get_global_position();

    auto tab_button_height = button_container->get_effective_minimum_size().y;

    auto default_theme = DefaultResource::get_singleton()->get_default_theme();

    auto theme_tab_bg = theme_override_tab_bg.value_or(default_theme->tab_container.styles["tab_background"]);

    auto theme_bg = theme_override_bg.value_or(default_theme->tab_container.styles["background"]);

    vs->draw_style_box(theme_bg, global_pos, size);
    vs->draw_style_box(theme_tab_bg, global_pos, {size.x, tab_button_height});

    for (const auto &btn : tab_buttons) {
        btn->theme_override_normal = default_theme->tab_container.styles["tab_button_normal"];
        btn->theme_override_hovered = btn->theme_override_normal;
        btn->theme_override_pressed = default_theme->tab_container.styles["tab_button_pressed"];
    }
}

void TabContainer::add_child(const std::shared_ptr<Node> &new_child) {
    Node::add_child(new_child);

    reload_tab_buttons();

    if (children.size() > 1) {
        set_current_tab(0);
    }
}

void TabContainer::add_child_at_index(const std::shared_ptr<Node> &new_child, uint32_t index) {
    Node::add_child_at_index(new_child, index);

    reload_tab_buttons();

    if (children.size() > 1) {
        set_current_tab(0);
    }
}

void TabContainer::reload_tab_buttons() {
    button_container->remove_all_children();
    tab_buttons.clear();
    tab_button_group->clear_buttons();

    for (int idx = 0; idx < children.size(); idx++) {
        auto button = std::make_shared<Button>();
        button->set_pressed_style_to_toggled(true);

        if (children[idx]->name.empty()) {
            button->set_text("TAB");
        } else {
            button->set_text(children[idx]->name);
        }

        button_container->add_child(button);
        tab_buttons.push_back(button);
        tab_button_group->add_button(button);

        auto callback = [this, idx](const bool toggled) {
            if (toggled) {
                current_tab = idx;
            }
        };
        button->connect_signal("toggled", callback);
        button->set_toggle_mode(true);
    }
}

void TabContainer::set_tab_disabled(bool disabled) {
    for (auto &btn : tab_buttons) {
        btn->set_disabled(disabled);
    }
}

} // namespace vecgui
