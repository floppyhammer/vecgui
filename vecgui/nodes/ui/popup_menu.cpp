#include "popup_menu.h"

#include <string>

#include "../../common/context.h"
#include "../../common/utils.h"
#include "../../resources/default_resource.h"
#include "../../servers/vector_server.h"
#include "../scene_tree.h"

namespace vecgui {

PopupMenu::PopupMenu() {
    type = NodeType::PopupMenu;

    margin_container_ = std::make_shared<MarginContainer>();
    margin_container_->name = "PopupMenu embedded margin container";
    margin_container_->set_anchor_flag(AnchorFlag::FullRect);
    add_embedded_child(margin_container_);

    scroll_container_ = std::make_shared<ScrollContainer>();
    scroll_container_->enable_hscroll(false);
    margin_container_->add_child(scroll_container_);

    vbox_container_ = std::make_shared<VBoxContainer>();
    vbox_container_->set_mouse_filter(MouseFilter::Pass);
    scroll_container_->add_child(vbox_container_);
}

void PopupMenu::draw() {
    if (!visible_) {
        return;
    }

    auto context = get_context();
    if (!context) {
        return;
    }

    auto vector_server = context->vector_server;

    NodeUi::draw();

    auto default_theme = context->default_resource->get_default_theme();

    auto theme_bg = theme_override_bg.value_or(default_theme->popup_menu.styles["background"]);

    vector_server->draw_style_box(theme_bg, get_global_position(), size);

    for (const auto &item : items_) {
        item->theme_override_normal = default_theme->popup_menu.styles["item_normal"];
    }
}

void PopupMenu::input(InputEvent &event) {
    if (!visible_) {
        return;
    }

    auto global_position = get_global_position();

    // If a popup menu is shown, it captures mouse events anyway.
    bool consume_flag = true;

    if (event.type == InputEventType::MouseButton) {
        auto args = event.args.mouse_button;

        // Hide menu.
        if (!RectF(global_position, global_position + size).contains_point(args.position)) {
            consume_flag = true;
            set_visibility(false);
        }
    }

    NodeUi::input(event);
}

void PopupMenu::adjust_layout() {
    size = size.max(calculated_minimum_size);

    float window_height = 0;
    if (tree_) {
        window_height = tree_->get_view_size().y;
    }

#ifdef VECGUI_USE_WINDOW
    auto context = get_context();
    auto render_context = context ? context->render_context : nullptr;
    auto window_builder = render_context ? render_context->get_window_builder() : nullptr;
    if (window_builder) {
        auto window = window_builder->get_window(get_window_index());
        if (!window.expired()) {
            window_height = window.lock()->get_logical_size().y;
        }
    }
#endif

    // Decide if it pops up or down based on available space.
    auto global_position = popup_position;

    float menu_width = std::max(size.x, margin_container_->get_effective_minimum_size().x);
    float menu_top_space = global_position.y;
    float menu_bottom_space = window_height - global_position.y - button_height;

    float min_menu_height = vbox_container_->get_effective_minimum_size().y + margin_container_->get_margin().top +
                            margin_container_->get_margin().bottom + 2;

    bool drop_down = true;
    float actual_menu_height = std::min(min_menu_height, menu_bottom_space);

    if (min_menu_height > menu_bottom_space) {
        if (menu_top_space > menu_bottom_space) {
            drop_down = false;
            actual_menu_height = std::min(min_menu_height, menu_top_space);
        }
    }

    if (drop_down) {
        set_position(popup_position + Vec2F{0, button_height});
        size = {menu_width, actual_menu_height};
    } else {
        set_position(popup_position - Vec2F{0, actual_menu_height});
        size = {menu_width, actual_menu_height};
    }

    // Ensure the embedded container matches the menu's size.
    if (margin_container_) {
        margin_container_->set_size(size);
    }
}

void PopupMenu::set_visibility(bool visible) {
    if (visible_ == visible) {
        return;
    }

    visible_ = visible;

    if (visible_) {
        // Trigger a relayout to handle positioning and sizing in adjust_layout().
        queue_relayout();
    } else {
        when_popup_hide();
    }
}

void PopupMenu::clear_items() {
    vbox_container_->remove_all_children();
    items_.clear();
    meta_.clear();
}

void PopupMenu::calc_minimum_size() {
    calculated_minimum_size = margin_container_->get_effective_minimum_size();
}

void PopupMenu::set_popup_position(Vec2F new_position, float new_button_height) {
    popup_position = new_position;
    button_height = new_button_height;
}

void PopupMenu::create_item(const std::string &text) {
    auto new_item = std::make_shared<Button>();
    new_item->set_mouse_filter(MouseFilter::Pass);
    new_item->set_text(text);

    vbox_container_->add_child(new_item);
    vbox_container_->set_separation(0);
    vbox_container_->set_mouse_filter(MouseFilter::Pass);

    uint32_t item_index = items_.size();

    auto callback = [item_index, this] {
        set_visibility(false);
        when_item_selected(item_index);
    };
    new_item->connect_signal("triggered", callback);

    items_.push_back(new_item);
    meta_.emplace_back();

    // Node::add_child() only queues into pending_children, and
    // Node::flush_pending_children() recurses into *newly added* children only.
    // A container that is already in the tree therefore never has its pending
    // children applied, so items added after construction would never appear.
    // Same idiom as TabContainer::reload_tab_buttons().
    vbox_container_->flush_pending_children();
}

void PopupMenu::set_item_meta(uint32_t item_index, std::string meta) {
    meta_[item_index] = std::move(meta);
}

void PopupMenu::set_item_height(float new_item_height) {
    item_height_ = new_item_height;
    // TODO
}

float PopupMenu::get_item_height() const {
    return item_height_;
}

int PopupMenu::get_item_count() const {
    return items_.size();
}

std::string PopupMenu::get_item_text(uint32_t item_index) const {
    return items_[item_index]->get_text();
}

std::string PopupMenu::get_item_mata(uint32_t item_index) const {
    return meta_[item_index];
}

void PopupMenu::connect_signal(const std::string &signal, const AnyCallable<void> &callback) {
    NodeUi::connect_signal(signal, callback);

    if (signal == "item_selected") {
        item_selected_callbacks.push_back(callback);
    }
    if (signal == "popup_hide") {
        popup_hide_callbacks.push_back(callback);
    }
}

void PopupMenu::when_item_selected(uint32_t item_index) {
    for (auto &callback : item_selected_callbacks) {
        try {
            callback.operator()<uint32_t>(std::move(item_index));
        } catch (std::bad_any_cast &) {
            Logger::error("Mismatched signal argument types!", "vecgui");
        }
    }
}

void PopupMenu::when_popup_hide() {
    for (auto &callback : popup_hide_callbacks) {
        try {
            callback();
        } catch (std::bad_any_cast &) {
            Logger::error("Mismatched signal argument types!", "vecgui");
        }
    }
}

} // namespace vecgui
