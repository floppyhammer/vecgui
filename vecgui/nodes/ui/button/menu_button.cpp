#include "menu_button.h"

#include "../container/scroll_container.h"
#include "../popup_menu.h"

namespace vecgui {

MenuButton::MenuButton() {
    type = NodeType::MenuButton;

    label->set_text("Menu Button");

    menu = std::make_shared<PopupMenu>();
    // Add a Node to disable transform propagation since a PopupMenu will always be in the global coordinates.
    auto node = std::make_shared<Node>();
    add_embedded_child(node);
    node->add_child(menu);

    menu->set_visibility(false);

    triggered_callbacks.emplace_back([this] {
        if (menu->get_item_count() == 0) {
            return;
        }
        auto global_pos = get_global_position();
        menu->set_popup_position(global_pos, size.y);
        menu->set_custom_minimum_size({size.x, 0});
        menu->set_visibility(true);
    });

    auto callback = [this](uint32_t item_index) { when_item_selected(item_index); };
    menu->connect_signal("item_selected", callback);
}

std::weak_ptr<PopupMenu> MenuButton::get_popup_menu() const {
    return menu;
}

void MenuButton::calc_minimum_size() {
    Button::calc_minimum_size();

    // Deliberately not widened to the popup's width. Adopting it means a single long
    // item forces the button - and every container holding it, e.g. a sidebar - to grow
    // to that width, even while the item is not selected. The popup keeps its own
    // natural width and simply overhangs the button, which is how dropdowns behave
    // elsewhere, so long entries stay fully readable without distorting the layout.
}

void MenuButton::connect_signal(const std::string& signal, const AnyCallable<void>& callback) {
    Button::connect_signal(signal, callback);

    if (signal == "item_selected") {
        selected_callbacks.push_back(callback);
    }
}

void MenuButton::select_item(uint32_t item_index) {
    when_item_selected(item_index);
}

std::string MenuButton::get_selected_item_text() const {
    if (selected_item_index.has_value()) {
        return menu->get_item_text(selected_item_index.value());
    }
    return "";
}

std::string MenuButton::get_selected_item_meta() const {
    if (selected_item_index.has_value()) {
        return menu->get_item_mata(selected_item_index.value());
    }
    return "";
}

std::optional<uint32_t> MenuButton::get_selected_item_index() const {
    return selected_item_index;
}

void MenuButton::when_item_selected(uint32_t item_index) {
    set_text(menu->get_item_text(item_index));
    selected_item_index = item_index;

    for (auto& callback : selected_callbacks) {
        try {
            callback.operator()<uint32_t>(std::move(item_index));
        } catch (std::bad_any_cast&) {
            Logger::error("Mismatched signal argument types!");
        }
    }
}

} // namespace vecgui
